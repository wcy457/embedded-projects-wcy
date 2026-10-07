#ifndef __MQ135_H
#define __MQ135_H

#include "main.h"

extern ADC_HandleTypeDef hadc1;
/* MQ-135 气体传感器，ADC 读取，返回估算 ppm（需实测标定） */
void     mq135_init(void);
uint16_t mq135_read_ppm(void);

#endif /* __MQ135_H */
