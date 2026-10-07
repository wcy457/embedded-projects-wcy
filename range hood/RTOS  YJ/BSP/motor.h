#ifndef __MOTOR_H
#define __MOTOR_H

#include "main.h"

extern TIM_HandleTypeDef htim1;

/* DRV8833 直流有刷电机驱动：TIM1_CH1 输出 PWM → AIN1，方向固定正转，nSLEEP 使能 */
void motor_init(void);
void motor_set_duty(uint16_t permille);        /* 0~1000‰ */
void motor_set_dir(uint8_t forward);           /* DRV8833 固定正转，空实现 */
void motor_enable(uint8_t on);                 /* 1=使能(nSLEEP 拉高) 0=待机 */

#endif /* __MOTOR_H */
