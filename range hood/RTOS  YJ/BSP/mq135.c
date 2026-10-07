#include "mq135.h"

#define MQ135_PPM_FULL_SCALE 400 /* ADC最大值4095对应气体浓度400ppm */
#define MQ135_SAMPLE_TIMES  8

/**
 * @brief  MQ?135空气质量传感器初始化
 * @note   CubeMX已提前配置好ADC1通道；MQ135上电需长时间预热读数才稳定
 */
 void mq135_init(void)
 {
	 HAL_ADCEx_Calibration_Start(&hadc1);//开启ADC自校准
 }

/**
 * @brief 读取MQ?135气体浓度，简易线性换算
 * @retval uint16_t 气体浓度值，单位ppm，范围：0 ~ 400
 * @warning 本函数仅做线性映射，未使用传感器标准响应曲线，精度有限，适合演示
 */
 uint16_t mq135_read_ppm(void)
 {
	 uint32_t sum = 0;
	 uint32_t raw;
	 
	 int i;
	 for(i = 0; i < MQ135_SAMPLE_TIMES; i++)
	 {
		 HAL_ADC_Start(&hadc1);
		 HAL_ADC_PollForConversion(&hadc1, 10);
		 raw = HAL_ADC_GetValue(&hadc1);
		 HAL_ADC_Stop(&hadc1);
		 
		 sum += raw;
		 HAL_Delay(2);
	 }
	 
	 raw = sum / MQ135_SAMPLE_TIMES;
	 
	 /*
     * 线性映射：12位ADC(0?4095) → 浓度(0?400ppm)
     * raw / 4096 = ppm / MQ135_PPM_FULL_SCALE
     * 后缀UL强制无符号长整型，避免整型溢出
     */
	 return (uint16_t)((raw * MQ135_PPM_FULL_SCALE) / 4096UL);
 }
