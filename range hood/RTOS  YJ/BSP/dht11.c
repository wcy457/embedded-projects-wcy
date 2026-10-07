#include "dht11.h"

#define DHT_GPIO_PORT DHT11_PORT
#define DHT_GPIO_PIN DHT11_PIN

/**
 * @brief 高精度微秒延时，依靠内核DWT周期计数器实现
 * @param us 需要延时的微秒数
 */
static void delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    /* 换算需要等待的CPU时钟节拍数 */
    uint32_t ticks = us * (SystemCoreClock / 1000000UL);
    while ((DWT->CYCCNT - start) < ticks)
    {
    }
}

/**
 * @brief 使能DWT周期计数器，用于us延时
 * @note 仅Cortex?M3/M4/M7内核支持CYCCNT
 */
static void dht_enable_dwt(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  /* 开启调试跟踪模块 */
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;              /* 使能周期计数器 */
    DWT->CYCCNT = 0;                                   /* 计数器清零 */
}

/**
 * @brief 将DHT11数据线配置为开漏输出模式，主机发送起始信号
 */
static void dht_pin_output(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = DHT_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_OD;   /* 开漏输出，外部上拉控制高电平 */
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT_GPIO_PORT, &gpio);
}

/**
 * @brief 将数据线切换成输入模式，读取DHT11返回的数据电平
 */
static void dht_pin_input(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = DHT_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT_GPIO_PORT, &gpio);
}

/**
 * @brief 读取DHT11的1位数据
 * @retval 0/1 正常数据；0xFF 读取超时失败
 */
static uint8_t dht_read_bit(void)
{
    uint32_t cnt = 0;
    /* 等待起始低电平结束，数据线被拉高 */
    while (HAL_GPIO_ReadPin(DHT_GPIO_PORT, DHT_GPIO_PIN) == GPIO_PIN_RESET)
    {
        if (++cnt > 500) return 0xFF;   /* 超时无响应，返回错误标记 */
    }

    delay_us(40);                       /* 延时40us后采样电平 */
    /* DHT11时序：高电平持续26?28us代表0；70us代表1 */
    uint8_t b = HAL_GPIO_ReadPin(DHT_GPIO_PORT, DHT_GPIO_PIN);

    /* 等待当前bit高电平段结束，数据线重新拉低 */
    cnt = 0;
    while (HAL_GPIO_ReadPin(DHT_GPIO_PORT, DHT_GPIO_PIN) == GPIO_PIN_SET)
    {
        if (++cnt > 500) return 0xFF;
    }
    return (b == GPIO_PIN_SET) ? 1 : 0;
}

/**
 * @brief 读取DHT11温湿度数据
 * @param temp_x10  输出温度值 ×10，例如25.0℃ → 250
 * @param humid_x10 输出湿度值 ×10，例如55.0% → 550
 * @retval 1 读取成功；0 失败（超时/校验错误）
 * @note  读取间隔建议大于2s，DHT11刷新率很低
 */
uint8_t dht11_read(int16_t *temp_x10, int16_t *humid_x10)
{
#if PROTEUS_SIM
    /* Proteus 仿真：无 DHT11 模型，返回固定温湿度 25.0℃ / 55.0% */
    *temp_x10  = 250;
    *humid_x10 = 550;
    return 1;
#else
    uint8_t data[5] = {0};   /* 5字节数据包：湿度整数、湿度小数、温度整数、温度小数、校验和 */
    uint8_t i, j, bit;

    dht_enable_dwt();

    /* 1. 主机发送起始信号：拉低数据线18ms，通知DHT11准备通信 */
    dht_pin_output();
    HAL_GPIO_WritePin(DHT_GPIO_PORT, DHT_GPIO_PIN, GPIO_PIN_RESET);
    delay_us(18000);
    HAL_GPIO_WritePin(DHT_GPIO_PORT, DHT_GPIO_PIN, GPIO_PIN_SET);
    delay_us(30);

    /* 2. 切换输入模式，等待DHT11应答信号：80us低电平 + 80us高电平 */
    dht_pin_input();
    j = 0;
    while (HAL_GPIO_ReadPin(DHT_GPIO_PORT, DHT_GPIO_PIN) == GPIO_PIN_RESET)
    {
        if (++j > 600) return 0;    /* 没有收到DHT11应答低电平，通信失败 */
    }
    j = 0;
    while (HAL_GPIO_ReadPin(DHT_GPIO_PORT, DHT_GPIO_PIN) == GPIO_PIN_SET)
    {
        if (++j > 600) return 0;    /* 应答高电平超时 */
    }

    /* 3. 连续读取40bit（5字节）温湿度数据 */
    for (i = 0; i < 5; i++)
    {
        data[i] = 0;
        for (j = 0; j < 8; j++)
        {
            bit = dht_read_bit();
            if (bit == 0xFF) return 0;   /* 某位读取超时，整体失败 */
            data[i] = (uint8_t)((data[i] << 1) | bit);
        }
    }

    /* 4. 校验和校验，验证数据是否传输正确 */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4])
    {
        return 0;
    }

    /* DHT11：湿度=整数.小数，温度=整数.小数，放大十倍保存避免浮点数 */
    *humid_x10 = (int16_t)(data[0] * 10 + data[1]);
    *temp_x10  = (int16_t)(data[2] * 10 + data[3]);

    return 1;
#endif /* !PROTEUS_SIM */
}
