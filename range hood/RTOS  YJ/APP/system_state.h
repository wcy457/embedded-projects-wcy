#ifndef __SYSTEM_STATE_H
#define __SYSTEM_STATE_H

#include "main.h"

typedef enum{
	MODE_OFF = 0,/* 关机模式 */
	MODE_MANUAL,/* 手动控制模式 */
	MODE_AUTO,/* 自动调速模式 */
	MODE_BACKFLOW/* 防回流保护模式 */
}system_mode_t;

typedef enum{
	GEAR_LOW = 0,
	GEAR_HIGH
}gear_t;

typedef struct{
	  system_mode_t mode;         /* 当前运行模式 */
    gear_t        gear;         /* 手动挡位 */
    uint16_t      target_rpm;   /* 电机目标转速 */
    uint16_t      actual_rpm;   /* 电机实测反馈转速 */
    uint16_t      pwm_duty;     /* 当前PWM占空比，单位千分比 0~1000‰ */
    int16_t       temperature;  /* 温度值，放大10倍存储（例25.5℃ → 255） */
    int16_t       humidity;     /* 湿度值，放大10倍存储 */
    uint16_t      gas_ppm;      /* 油烟/可燃气体浓度，单位 ppm */
    uint8_t       cooking_event; /* 烹饪事件触发标志位 */
    uint8_t       backflow_active;/* 防回流功能激活标志位 */
}system_state_t;

extern system_state_t g_state;

void state_init(void);
void state_lock(void);
void state_unlock(void);

#endif
