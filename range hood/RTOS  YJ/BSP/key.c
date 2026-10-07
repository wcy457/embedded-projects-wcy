#include "key.h"

#define KEY_SCAN_MS  10// 按键扫描任务调用周期 10ms
#define KEY_DEBOUNCE_CNT 2  // 连续2次采样电平相同判定稳定，消抖时长 = 2*10 = 20ms
#define KEY_LONG_MS  1000  // 长按触发阈值 1000ms

/**
 * @brief  单个按键状态控制结构体
 * @note   放在FreeRTOS多任务环境下，按键任务10ms周期扫描，
 *         扫描得到的短按/长按事件通过消息队列发送给系统主状态机任务
 */
typedef struct{
	uint8_t stable;         // 消抖稳定后的按键电平状态：1=按下，0=释放
	uint8_t raw_prev;       // 上一轮读取到的GPIO原始电平采样值
	uint8_t db_cnt;         // 软件消抖采样计数器
	uint16_t press_ms;      // 按键持续按下累计时长(ms)
	uint8_t long_reported;  // 长按上报标记：1=已经上报过长按事件，0=未上报
}key_ctrl_t;

static key_ctrl_t s_keys[KEY_NUM];

static GPIO_TypeDef *const s_ports[KEY_NUM] = {
	KEY1_PORT, KEY2_PORT, KEY3_PORT, KEY4_PORT
};

// 各个按键对应的引脚号数组
static const uint16_t s_pins[KEY_NUM] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN
};

/**
 * @brief  按键GPIO初始化函数
 * @param  无
 * @retval 无
 * @note   硬件接法：引脚开启内部上拉，按键另一端接GND，低电平代表按键按下
 *         调用时机：main函数、FreeRTOS任务启动之前完成初始化
 */
void key_init(void)
{
	GPIO_InitTypeDef gpio = {0};
	int i;
	for(i = 0; i < KEY_NUM; i++)
	{
		s_keys[i].stable = 0;
    s_keys[i].raw_prev = 0;
    s_keys[i].db_cnt = 0;
    s_keys[i].press_ms = 0;
    s_keys[i].long_reported = 0;
	}
	
	gpio.Mode = GPIO_MODE_INPUT;    // GPIO配置成输入模式
  gpio.Pull = GPIO_PULLUP;         // 开启芯片内部上拉电阻
	
	for(i = 0; i < KEY_NUM; i++)
	{
		gpio.Pin = s_pins[i];
		HAL_GPIO_Init(s_ports[i], &gpio);
	}
}

/**
 * @brief  单按键扫描，软件消抖 + 短按/长按事件识别
 * @param  id: 按键编号，枚举类型 key_id_t
 * @retval key_evt_t
 *          KEY_EVT_NONE    无按键事件
 *          KEY_EVT_SHORT   短按事件，按键松开瞬间上报一次
 *          KEY_EVT_LONG    长按事件，按下满1000ms瞬间上报一次
 * @note   【FreeRTOS使用注意事项】
 *         1. 必须放在周期10ms的按键任务中循环调用，禁止在中断中调用本函数
 *         2. 返回得到按键事件后，建议使用xQueueSend发送按键事件至消息队列，
 *            交由主业务任务处理，不要在按键扫描任务内执行耗时业务逻辑
 */
key_evt_t key_scan(key_id_t id)
{
	  key_ctrl_t *k = &s_keys[id];
    uint8_t raw;
    key_evt_t evt = KEY_EVT_NONE;

    /* 读取引脚电平：低电平按下 */
    raw = (HAL_GPIO_ReadPin(s_ports[id], s_pins[id]) == GPIO_PIN_RESET) ? 1 : 0;

    if (raw == k->raw_prev)
    {
        if (k->db_cnt < KEY_DEBOUNCE_CNT)
        {
            k->db_cnt++;
        }
        else
        {
            if (raw != k->stable)
            {
                k->stable = raw;
                if (raw)
                {
                    /* 按键刚按下 */
                    k->press_ms = 0;
                    k->long_reported = 0;
                }
                else
                {
                    /* 按键松开，未触发过长按 → 短按事件 */
                    if (!k->long_reported)
                    {
                        evt = KEY_EVT_SHORT;
                    }
                }
            }

            if (raw)
            {
                k->press_ms += KEY_SCAN_MS;
                /* 到达长按阈值，并且没有上报过 */
                if (k->press_ms >= KEY_LONG_MS && !k->long_reported)
                {
                    k->long_reported = 1;
                    evt = KEY_EVT_LONG;
                }
            }
        }
    }
    else
    {
        /* 电平跳变，抖动，计数器清零 */
        k->db_cnt = 0;
    }

    k->raw_prev = raw;
    return evt;
}
