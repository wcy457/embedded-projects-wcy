#include "DHT11.h"
#include "Delay.h"

DHT11_Data_TypeDef DHT11_Data;

// DHT11 数据脚 = PB10
#define DHT11_PIN     GPIO_Pin_10
#define DHT11_PORT    GPIOB
#define DHT11_RCC     RCC_APB2Periph_GPIOB

//输出模式
static void DHT11_OUT(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = DHT11_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_PORT, &gpio);
}

//输入模式
static void DHT11_IN(void)
{
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Pin = DHT11_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(DHT11_PORT, &gpio);
}

void DHT11_Init(void)
{
    RCC_APB2PeriphClockCmd(DHT11_RCC, ENABLE);
    DHT11_OUT();
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
}

//读一个字节
static u8 DHT11_Read_Byte(void)
{
    u8 i, dat = 0;
    for(i=0; i<8; i++)
    {
        while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0);
        Delay_us(40);
        if(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1)
        {
            dat |= (1 << (7 - i));
        }
        while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1);
    }
    return dat;
}

u8 DHT11_ReadData(DHT11_Data_TypeDef *data)
{
    u8 buf[5] = {0};
    u8 i;

    DHT11_OUT();
    GPIO_ResetBits(DHT11_PORT, DHT11_PIN);
    Delay_ms(20);
    GPIO_SetBits(DHT11_PORT, DHT11_PIN);
    Delay_us(30);

    DHT11_IN();
    if(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0)
    {
        while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 0);
        while(GPIO_ReadInputDataBit(DHT11_PORT, DHT11_PIN) == 1);

        for(i=0; i<5; i++)
        {
            buf[i] = DHT11_Read_Byte();
        }

        if(buf[0] + buf[1] + buf[2] + buf[3] == buf[4])
        {
            data->humi_int = buf[0];
            data->humi_dec = buf[1];
            data->temp_int = buf[2];
            data->temp_dec = buf[3];
            return 0;
        }
    }
    return 1;
}
