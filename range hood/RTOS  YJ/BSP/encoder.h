#ifndef __ENCODER_H
#define __ENCODER_H

#include "main.h"

extern TIM_HandleTypeDef htim3;
/*
 * TIM3 编码器模式（x4 计数），测速：
 * 速度计算任务周期性调用 encoder_get_delta() 取计数值并清零，
 * 再用 encoder_to_rpm() 换算转速。
 */
void     encoder_init(void);
int32_t  encoder_get_delta(void);                    /* 读取并清零计数值 */
uint16_t encoder_to_rpm(int32_t delta, uint32_t dt_ms);

#endif /* __ENCODER_H */
