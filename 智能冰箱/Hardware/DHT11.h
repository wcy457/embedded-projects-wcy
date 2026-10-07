#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f10x.h"

typedef struct
{
    u8 humi_int;    //湿度整数
    u8 humi_dec;    //湿度小数
    u8 temp_int;    //温度整数
    u8 temp_dec;    //温度小数
} DHT11_Data_TypeDef;

extern DHT11_Data_TypeDef DHT11_Data;

void DHT11_Init(void);
u8 DHT11_ReadData(DHT11_Data_TypeDef *data);

#endif
