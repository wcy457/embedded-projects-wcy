#ifndef __APP_CONFIG_H
#define __APP_CONFIG_H

#include <stdint.h>

/* ================= 仿真开关 ================= */
#define PROTEUS_SIM         1           /* 1=Proteus 仿真（OLED→串口、DHT11 固定值）；烧真机前改成 0 */

/* ================= 系统时钟 ================= */
#define SYS_CLOCK_HZ        72000000UL

/* ================= 任务周期（ms） ================= */
#define PERIOD_KEY_MS       10
#define PERIOD_SPEED_MS     50
#define PERIOD_MOTOR_MS     50
#define PERIOD_FUSION_MS    100
#define PERIOD_BACKFLOW_MS  100
#define PERIOD_SENSOR_MS    500
#define PERIOD_UI_MS        200

/* ================= 任务优先级（数字越大优先级越高） ================= */
#define PRIO_KEY            6
#define PRIO_SPEED          5
#define PRIO_MOTOR          5
#define PRIO_FUSION         4
#define PRIO_BACKFLOW       4
#define PRIO_SENSOR         2
#define PRIO_UI             1

/* ================= 任务栈大小（word） ================= */
#define STACK_KEY           128
#define STACK_SPEED         256
#define STACK_MOTOR         128
#define STACK_FUSION        256
#define STACK_BACKFLOW      128
#define STACK_SENSOR        256
#define STACK_UI            256

/* ================= 电机 / 编码器 ================= */
#define PWM_PERMILLE_MAX    1000        /* 占空比 0~1000‰ */
#define ENCODER_PPR         13          /* 霍尔编码器每转脉冲数（电机轴） */
#define GEAR_RATIO          1           /* 减速比 */
#define LOW_RPM             190         /* 低档目标转速 */
#define HIGH_RPM            220         /* 高档目标转速 */

/* ================= 融合算法参数 ================= */
#define FUSION_W_T          2           /* 温度权重 ×10 */
#define FUSION_W_H          2           /* 湿度权重 ×10 */
#define FUSION_W_G          6           /* 气体权重 ×10 */
#define BASE_TEMP           200         /* T_base=20.0℃ ×10 */
#define BASE_HUMID          400         /* H_base=40.0% ×10 */
#define BASE_GAS            80          /* G_base=80ppm */
#define RANGE_TEMP          200         /* 温度量程 ×10 */
#define RANGE_HUMID         400         /* 湿度量程 ×10 */
#define RANGE_GAS           220         /* 气体量程 ppm */
#define PWM_BASE_PERMILLE   10          /* 最小 1% */

/* ================= PID 参数（手动模式） ================= */
#define PID_KP              2.0f
#define PID_KI              0.05f
#define PID_KD              0.0f
#define PID_INTEGRAL_MAX    500.0f      /* 积分限幅，防积分饱和 */
#define PID_OUT_MAX         300         /* 输出增量限幅 */

/* ================= 防回流动态阈值 ================= */
#define BACKFLOW_NORMAL_TH  150         /* 正常触发阈值 ppm */
#define BACKFLOW_DEBOUNCE   120         /* 防抖阈值（低于此才关闭） */

/* ================= 自动模式 ================= */
#define AUTO_MIN_RUN_MS     60000       /* 最小运行 60s */
#define AUTO_EXIT_DELAY_MS  10000       /* 事件结束延迟 10s 退出 */

/* ================= USART / DMA ================= */
#define UART_RX_BUF_SIZE    128         /* 接收缓冲 128 字节 */

/* ================= 引脚定义 ================= */
#include "stm32f1xx_hal.h"

#define LED_PORT            GPIOC
#define LED_PIN             GPIO_PIN_13

#define MOTOR_AIN1_PORT     GPIOB
#define MOTOR_AIN1_PIN      GPIO_PIN_12
#define MOTOR_AIN2_PORT     GPIOB
#define MOTOR_AIN2_PIN      GPIO_PIN_13
#define MOTOR_STBY_PORT     GPIOB
#define MOTOR_STBY_PIN      GPIO_PIN_14

#define DHT11_PORT          GPIOA
#define DHT11_PIN           GPIO_PIN_1

#define KEY1_PORT           GPIOB
#define KEY1_PIN            GPIO_PIN_0
#define KEY2_PORT           GPIOB
#define KEY2_PIN            GPIO_PIN_1
#define KEY3_PORT           GPIOB
#define KEY3_PIN            GPIO_PIN_10
#define KEY4_PORT           GPIOB
#define KEY4_PIN            GPIO_PIN_11

#endif /* __APP_CONFIG_H */
