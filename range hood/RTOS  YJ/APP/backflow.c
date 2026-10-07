#include "backflow.h"
#include "app_config.h"

/* 回流检测激活状态标志 */
static uint8_t s_active = 0;

/**
 * @brief 油烟回流检测模块初始化
 * @note 系统上电或者模块复位时调用，清除回流激活状态
 */
void backflow_init(void)
{
	s_active = 0;
}

/**
 * @brief 回流检测状态更新，采用滞回比较(施密特)消除传感器抖动
 * @param gas_ppm 气体传感器滤波后浓度值，单位ppm
 * @return uint8_t 回流状态，1=存在油烟回流，0=无油烟回流
 * @note 滞回逻辑：
 *  未激活：气体浓度大于高阈值才触发激活；
 *  已激活：气体浓度小于低阈值才解除激活；
 *  高低阈值不同，避免传感器小幅噪声造成状态频繁跳变。
 */
uint8_t backflow_update(uint16_t gas_ppm)
{
	if(!s_active)
	{
		if(gas_ppm > BACKFLOW_NORMAL_TH)
		{
			s_active = 1;
		}
	}
	else
	{
		/* 当前已经激活，浓度下降到防抖低阈值以下，才清除回流状态 */
		if (gas_ppm < BACKFLOW_DEBOUNCE)
		{
				s_active = 0;
		}
	}
	
	return s_active;
}
