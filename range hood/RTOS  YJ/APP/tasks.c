#include "tasks.h"         // 当前C文件对应的头文件
#include "system_state.h"  // 全局系统状态g_state，模式、转速、传感器数据、状态锁
#include "fusion.h"        // 多传感器融合算法
#include "state_machine.h" // 自动模式状态机
#include "pid.h"           // PID转速闭环控制
#include "backflow.h"      // 烟道回流检测模块
#include <stdio.h>         // snprintf，仅仿真串口打印调试
#include "oled.h"          // OLED屏幕驱动
#include "dht11.h"         // DHT11温湿度传感器驱动
#include "mq135.h"         // MQ135气体传感器驱动
#include "motor.h"         // 电机PWM输出驱动
#include "encoder.h"       // 编码器转速采集
#include "key.h"           // 按键扫描驱动
#include "uart_dma.h"      // 串口DMA驱动
#include "FreeRTOS.h"      // FreeRTOS基础头文件
#include "task.h"          // FreeRTOS任务相关API

static TaskHandle_t h_key;       // 按键任务句柄
static TaskHandle_t h_speed;     // 转速采集PID任务句柄
static TaskHandle_t h_motor;     // 电机PWM输出任务句柄
static TaskHandle_t h_fusion;    // 传感器融合+自动状态机任务句柄
static TaskHandle_t h_backflow;  // 烟道回流检测任务句柄
static TaskHandle_t h_sensor;    // 传感器采集任务句柄
static TaskHandle_t h_ui;        // OLED/串口UI显示任务句柄

static pid_t    s_speed_pid;        // 风机转速PID控制器实例，手动模式下做转速闭环
static uint16_t s_pid_base = 400;   // PID开环前馈千分占空比，基础输出；PID输出是修正量，叠加在前馈上得到最终PWM
/**
 * @brief 按键回调：循环切换系统工作模式
 * 模式流转：关机MODE_OFF → 手动MODE_MANUAL → 自动MODE_AUTO → 回流MODE_BACKFLOW → 关机MODE_OFF
 * @note state_lock保护全局g_state，多任务同时访问全局变量会造成数据撕裂；修改完成后必须解锁
 */
static void key_change_mode(void)
{
	state_lock();// 加锁，进入临界区，独占访问全局系统状态
	
	switch(g_state.mode)
	{
		case MODE_OFF:      g_state.mode = MODE_MANUAL;   break; // 关机切换为手动模式
    case MODE_MANUAL:   g_state.mode = MODE_AUTO;     break; // 手动切换为自动模式
    case MODE_AUTO:     g_state.mode = MODE_BACKFLOW; break; // 自动切换为烟道回流检测模式
    case MODE_BACKFLOW: g_state.mode = MODE_OFF;      break; // 回流检测切换为关机
	}
	
	// 如果切换到自动模式，启动自动模式状态机；其余模式复位状态机，清除内部状态
	if(g_state.mode == MODE_AUTO) auto_fsm_start();
	else auto_fsm_reset();
	
	// 切到关机模式，直接将PWM占空比置0，风机停止转动
	if(g_state.mode == MODE_OFF) g_state.pwm_duty = 0;
	
	state_unlock();
}

/**
 * @brief 设置手动模式风机档位，高档GEAR_HIGH / 低档GEAR_LOW
 * @param gear 档位枚举
 */
static void key_set_gear(gear_t gear)
{
	state_lock();
	g_state.gear = gear;
	state_unlock();
}

/**
 * @brief 电源按键处理函数，实现整机开关机
 * 关机状态按下：进入手动模式；任意工作状态按下：直接关机复位
 */
static void key_power_toggle(void)
{
	state_lock();
	
	if(g_state.mode == MODE_OFF)
	{
		g_state.mode = MODE_MANUAL;
	}
	else
	{
		g_state.mode = MODE_OFF;     // 运行状态按下电源，设置为关机
		g_state.pwm_duty = 0;       // PWM输出清零，风机停止
		auto_fsm_reset();           // 复位自动模式状态机，防止残留状态
	}
	
	state_unlock();
}

/**
 * @brief 扫描4个按键，解析按键事件，分发调用对应处理函数
 * KEY1：模式切换；KEY2：高档；KEY3：低档；KEY4：开关机
 */
static void handle_keys(void)
{
	key_evt_t e1 = key_scan(KEY_1);
	key_evt_t e2 = key_scan(KEY_2);
	key_evt_t e3 = key_scan(KEY_3);
	key_evt_t e4 = key_scan(KEY_4);
	
	if (e1 == KEY_EVT_SHORT) key_change_mode();      // KEY1短按，切换工作模式
  if (e2 == KEY_EVT_SHORT) key_set_gear(GEAR_HIGH);// KEY2短按，设置高档
  if (e3 == KEY_EVT_SHORT) key_set_gear(GEAR_LOW); // KEY3短按，设置低档
  if (e4 == KEY_EVT_SHORT) key_power_toggle();     // KEY4短按，电源开关机
}

/**
 * @brief 手动模式PID计算一步
 * @param target 目标转速RPM
 * @param actual 编码器反馈实际转速RPM
 * @return uint16_t 千分PWM占空比0~PWM_PERMILLE_MAX
 * @note 控制结构：前馈基准值 + PID修正输出；做输出限幅保护，PWM不能小于0，不能大于最大值
 */
static uint16_t manual_pid_step(uint16_t target, uint16_t actual)
{
	int32_t out  = pid_update(&s_speed_pid, (int32_t)target, (int32_t)actual); // PID计算得到转速偏差修正量
	int32_t pwm  = (int32_t)s_pid_base + out;                                  // 前馈基础PWM叠加PID修正量
	if (pwm < 0)                  pwm = 0;                                      // 输出下限保护，不能负占空比
	if (pwm > PWM_PERMILLE_MAX)   pwm = PWM_PERMILLE_MAX;                       // 输出上限保护，不能超过最大占空比
	return (uint16_t)pwm;
}

/* ================= 各任务 ================= */

/**
 * @brief 按键扫描任务，固定周期PERIOD_KEY_MS(10ms)
 * @param arg 任务传入参数，本项目不使用，强制(void)arg消除编译警告
 * @note vTaskDelayUntil实现严格固定周期，不会被任务执行耗时影响周期；for(;;)RTOS任务死循环
 */
static void vKeyTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	(void)arg;
// 强制把未使用的入参arg做类型转换，消除编译器“未使用参数”警告
	for(;;)
	{
		handle_keys();
		vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_KEY_MS));
	}
}

/**
 * @brief 速度采集与PID闭环任务，周期PERIOD_SPEED_MS(50ms)
 * 功能：读取编码器脉冲增量，换算实际RPM；手动模式下根据档位设置目标转速，执行PID闭环，更新全局pwm_duty
 */
static void vSpeedTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	int32_t delta;
	uint16_t rpm, target, pwm;
	(void)arg;
	for(;;)
	{
		delta = encoder_get_delta();                     // 获取本周期编码器脉冲增量
    rpm   = encoder_to_rpm(delta, PERIOD_SPEED_MS); // 根据脉冲增量和任务周期换算风机实际转速

    state_lock();
		g_state.actual_rpm = rpm;
		if(g_state.mode == MODE_MANUAL)
		{
			target = (g_state.gear == GEAR_LOW) ? LOW_RPM : HIGH_RPM; // 根据档位选择目标转速
			g_state.target_rpm = target;
      pwm = manual_pid_step(target, rpm);                       // PID计算PWM输出
      g_state.pwm_duty = pwm;                                   // 更新全局PWM占空比
		}
		
		state_unlock();

    vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_SPEED_MS));
	}
}

/**
 * @brief 电机输出任务，周期PERIOD_MOTOR_MS(50ms)
 * 只负责硬件输出：读取全局pwm_duty，调用底层motor_set_duty设置硬件PWM；算法逻辑和硬件输出解耦
 */
static void vMotorTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	uint16_t duty;
	(void)arg;
	for(;;)
	{
		state_lock();
		duty = g_state.pwm_duty; // 读取全局千分占空比
		state_unlock();
		motor_set_duty(duty);    // 将占空比写入定时器，输出硬件PWM波形

		vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_MOTOR_MS));
	}
}

/**
 * @brief 传感器融合 + 自动模式状态机任务，周期PERIOD_FUSION_MS(100ms)
 * 1、读取全局温湿度、气体浓度；
 * 2、调用fusion_compute做多传感器融合计算得到原始PWM；
 * 3、判断烹饪事件（气体浓度超过阈值）；
 * 4、MODE_AUTO模式运行auto_fsm_step状态机，更新全局pwm_duty
 */
static void vFusionTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	
	int16_t t,h;
	uint16_t g, pwm, auto_pwm;
	uint8_t cooking;
	(void)arg;
	for(;;)
	{
		state_lock();
		t = g_state.temperature;
		h = g_state.humidity;
		g = g_state.gas_ppm;
		state_unlock();

		pwm     = fusion_compute(t, h, g);                         // 多传感器融合计算PWM
		cooking = (g > COOKING_GAS_THRESHOLD) ? 1 : 0;            // 判断烹饪事件，气体大于阈值判定正在做饭

		state_lock();
		g_state.cooking_event = cooking;                           // 保存烹饪事件标志到全局状态
		if (g_state.mode == MODE_AUTO)                             // 只有自动模式才运行状态机
		{
				auto_pwm = auto_fsm_step(pwm, cooking);               // 执行自动模式状态机逻辑
				g_state.pwm_duty = auto_pwm;                          // 更新风机输出占空比
		}
		state_unlock();

		vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_FUSION_MS));
    }
}

/**
 * @brief 烟道回流检测任务，周期PERIOD_BACKFLOW_MS(100ms)
 * 读取气体浓度，执行滞回回流检测；MODE_BACKFLOW模式下根据回流标志控制风机输出，抵御公共烟道倒灌油烟
 */
static void vBackflowTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	uint16_t g;
	uint8_t active;
	(void)arg;
	for(;;)
	{
		state_lock();
		g = g_state.gas_ppm;
		state_unlock();
		
		active = backflow_update(g);// 执行滞回检测，得到回流标志1/0
		
		state_lock();
		g_state.backflow_active = active;
		if (g_state.mode == MODE_BACKFLOW)
			{
				/* 回流模式：检测到回流输出BACKFLOW_DUTY；没有回流PWM置0，风机停机 */
				g_state.pwm_duty = active ? BACKFLOW_DUTY : 0;
			}
		state_unlock();
			
		vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_BACKFLOW_MS));
	}
}

/**
 * @brief 传感器采集任务，周期PERIOD_SENSOR_MS(500ms)
 * 读取MQ135气体、DHT11温湿度；DHT11是单总线时序器件，读取时进入临界区关闭中断，防止时序被RTOS打断
 * DHT读取失败，温湿度清零，避免无效数据参与运算
 */
static void vSensorTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	int16_t t, h;
	uint16_t g;
	uint8_t ok;
	(void)arg;
	for(;;)
	{
		g = mq135_read_ppm();                          // 读取MQ?135油烟气体浓度

		taskENTER_CRITICAL();                          // 进入临界区，关闭全局中断，保护DHT11时序
		ok = dht11_read(&t, &h);                       // 读取DHT11温湿度
		taskEXIT_CRITICAL();                           // 退出临界区，开启中断

		if (!ok) { t = 0; h = 0; }                     // DHT11读取失败，温湿度清零

		state_lock();
		g_state.gas_ppm     = g;
		g_state.temperature = t;
		g_state.humidity    = h;
		state_unlock();

		vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_SENSOR_MS));
	}
}

/**
 * @brief UI刷新任务，周期PERIOD_UI_MS(200ms)
 * PROTEUS_SIM宏开启：仿真环境串口打印调试信息；
 * 未开启：实物OLED屏幕，显示工作模式、风机转速、温度、湿度、气体浓度
 */
static void vUiTask(void *arg)
{
	TickType_t last = xTaskGetTickCount();
	system_mode_t mode;
	uint16_t rpm, gas, target, duty;
	int16_t temp,humid;
	const char *m;
	(void)arg;
	for(;;)
	{
		state_lock();
		mode   = g_state.mode;
		rpm    = g_state.actual_rpm;
		target = g_state.target_rpm;
		duty   = g_state.pwm_duty;
		temp   = g_state.temperature;
		humid  = g_state.humidity;
		gas    = g_state.gas_ppm;
		state_unlock();
		
		/* 将枚举模式转为显示字符串 */
		m = "OFF";
		if (mode == MODE_MANUAL)        m = "MANU";
		else if (mode == MODE_AUTO)     m = "AUTO";
		else if (mode == MODE_BACKFLOW) m = "BACK";
		
		#if PROTEUS_SIM
        /* Proteus仿真模式，snprintf格式化字符串，串口输出调试信息 */
        {
            char line[96];
            int n = snprintf(line, sizeof(line),
                             "[%s] TGT=%u RPM=%u PWM=%u  T=%d.%d H=%d.%d GAS=%u\r\n",
                             m, target, rpm, duty,
                             temp / 10, temp % 10, humid / 10, humid % 10, gas);
            if (n > 0) HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, 100);
        }
#else
        /* 硬件OLED屏幕，分页清屏，绘制模式、转速、温湿度、气体 */
        oled_clear_page(0);
        oled_draw_string(0, 0, m);

        oled_clear_page(1);
        oled_draw_string(1, 0, "RPM:");
        oled_draw_int(1, 24, rpm);

        oled_clear_page(2);
        oled_draw_string(2, 0, "T:");
        oled_draw_int(2, 12, temp / 10);
        oled_draw_string(2, 54, "H:");
        oled_draw_int(2, 66, humid / 10);

        oled_clear_page(3);
        oled_draw_string(3, 0, "GAS:");
        oled_draw_int(3, 24, gas);
#endif
        vTaskDelayUntil(&last, pdMS_TO_TICKS(PERIOD_UI_MS));
		}
}

/**
 * @brief 系统任务初始化入口函数，main函数中vTaskStartScheduler()之前调用
 * 1、初始化PID、自动状态机、回流检测模块；
 * 2、依次创建全部7个FreeRTOS任务，配置任务函数、任务名、栈大小、入口参数、任务优先级、任务句柄
 */
void tasks_init(void)
{
	pid_init(&s_speed_pid, PID_KP, PID_KI, PID_KD, -PID_OUT_MAX, PID_OUT_MAX); // PID参数初始化，设置Kp Ki Kd、输出正负限幅
  auto_fsm_init();        // 自动模式状态机初始化
  backflow_init();        // 回流检测模块初始化
	
	xTaskCreate(vKeyTask,      "key",     STACK_KEY,      NULL, PRIO_KEY,      &h_key);
	xTaskCreate(vSpeedTask,    "speed",   STACK_SPEED,    NULL, PRIO_SPEED,    &h_speed);
	xTaskCreate(vMotorTask,    "motor",   STACK_MOTOR,    NULL, PRIO_MOTOR,    &h_motor);
	xTaskCreate(vFusionTask,   "fusion",  STACK_FUSION,   NULL, PRIO_FUSION,   &h_fusion);
	xTaskCreate(vBackflowTask, "backflow",STACK_BACKFLOW, NULL, PRIO_BACKFLOW, &h_backflow);
	xTaskCreate(vSensorTask,   "sensor",  STACK_SENSOR,   NULL, PRIO_SENSOR,   &h_sensor);
	xTaskCreate(vUiTask,       "ui",      STACK_UI,       NULL, PRIO_UI,       &h_ui);
}

// 堆内存分配失败钩子函数
void vApplicationMallocFailedHook(void)
{
    // 堆内存不足，任务创建失败，此处死循环，可做故障报警
    while(1);
}
