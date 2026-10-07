#include "state_machine.h"
#include "app_config.h"
#include "FreeRTOS.h"
#include "task.h"

/* 自动模式最小运行千分占空比，300‰ = 30%占空比 */
#define MIN_RUN_PWM     300

/**
 * @brief 油烟机自动模式状态枚举
 * AUTO_STANDBY ：待机状态，风机停止输出
 * AUTO_MIN_RUN ：预运行状态，输出最小转速，等待烹饪事件
 * AUTO_ADJUST  ：调节运行状态，依据传感器融合PWM动态调速
 */
typedef enum{
	AUTO_STANDBY = 0,
	AUTO_MIN_RUN,
	AUTO_ADJUST
}auto_fsm_state_t;

static auto_fsm_state_t s_st;/* 状态机当前状态 */
static TickType_t s_t0;/* 时间戳，记录状态跳转时刻的系统tick，用于计时 */
static uint8_t s_prev_cooking;/* 历史标记：是否曾经检测到烹饪事件 */

/**
 * @brief 自动模式状态机初始化
 * @note 上电调用，初始化全部静态状态变量
 */
void auto_fsm_init(void)
{
	s_st = AUTO_STANDBY;
	s_t0 = 0;
	s_prev_cooking = 0;
}

/**
 * @brief 启动自动模式，进入预运行状态
 * @note 用户打开自动模式时调用，记录当前tick作为计时起点
 */
void auto_fsm_start(void)
{
	s_st = AUTO_MIN_RUN;
	s_t0 = xTaskGetTickCount();
	s_prev_cooking = 0;
}

/**
 * @brief 强制复位状态机，直接切回待机
 * @note 用户关闭自动模式调用，风机停止工作
 */
void auto_fsm_reset(void)
{
    s_st = AUTO_STANDBY;
}

/**
 * @brief 自动模式状态机周期处理函数，任务内循环调用
 * @param fusion_pwm     fusion融合算法输出千分占空比(0~1000‰)
 * @param cooking_event  烹饪事件标志，1=检测到烹饪，0=无烹饪
 * @return uint16_t      返回风机输出千分PWM占空比，0代表风机停机
 */
uint16_t auto_fsm_step(uint16_t fusion_pwm,uint8_t cooking_event)
{
	TickType_t now = xTaskGetTickCount();/* 获取当前FreeRTOS系统滴答计数 */
	TickType_t elapsed = now - s_t0;/* 计算自计时起点过去的时间(tick) */
	
	switch (s_st)
	{
		/* 待机状态：后台依旧做传感器计算，风机输出0停止 */
		case AUTO_STANDBY:
			return 0;
		
		/* 预运行：输出最小转速，等待烹饪事件触发 */
		case AUTO_MIN_RUN:
			if(cooking_event)
			{
				/* 捕获烹饪事件，跳转至动态调节状态 */
				s_st = AUTO_ADJUST;
				s_prev_cooking = 1;
			}
			else if(elapsed >= pdMS_TO_TICKS(AUTO_MIN_RUN_MS))
			{
				s_st = AUTO_STANDBY;
			}
			return MIN_RUN_PWM;
			
		/* 动态调速状态，使用融合PWM控制风机 */
		case AUTO_ADJUST:
			if(cooking_event)
			{
				s_prev_cooking = 1;
				return fusion_pwm;
			}
			
			/* 烹饪事件消失，开启延时退出逻辑，不立刻关机 */
			if(s_prev_cooking)
			{
				s_t0 = now;
				s_prev_cooking = 0;
			}
			
			if (elapsed >= pdMS_TO_TICKS(AUTO_EXIT_DELAY_MS))
        {
            /* 延时时间到达，切待机，风机停机 */
            s_st = AUTO_STANDBY;
            return 0;
        }
        return fusion_pwm;              /* 延时排风阶段，维持当前风机转速 */

    default:
        /* 异常状态保护，风机停机 */
        return 0;
	}
}
