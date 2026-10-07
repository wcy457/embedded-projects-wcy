#include "encoder.h"

/**
 * @brief 编码器定时器初始化并开启计数
 * @note  使用TIM3编码器模式，双通道全通道捕获(CH1+CH2)，4倍频计数
 *         调用时机：main函数，FreeRTOS启动之前初始化硬件
 */
 void encoder_init(void)
 {
	 /* 启动定时器编码器接口，TIM_CHANNEL_ALL = CH1+CH2 双通道 */
	 HAL_TIM_Encoder_Start(&htim3, TIM_CHANNEL_ALL);
	 /* 将定时器计数器初始值清零 */
	 __HAL_TIM_SET_COUNTER(&htim3, 0);
 }

/**
 * @brief 读取并清零编码器增量值
 * @return int32_t 上一次读取到本次读取之间编码器的计数变化量
 *         >0：电机正转；<0：电机反转
 * @note  必须固定周期调用（例如50ms），读出delta之后立刻清零计数器
 */
int32_t encoder_get_delta(void)
{
	int32_t c = (int32_t)__HAL_TIM_GET_COUNTER(&htim3);
	__HAL_TIM_SET_COUNTER(&htim3, 0);
	return c;
}

/**
 * @brief 将编码器增量计数换算成转速 RPM（转/分钟）
 * @param delta   encoder_get_delta()返回的计数值增量
 * @param dt_ms   两次读取编码器的时间间隔，单位ms
 * @retval uint16_t 转速 rpm(转轴一分钟之内旋转多少整圈)，范围0?65535，取绝对值
 *
 * 计算公式推导：
 * 转速(r/min) = 增量 * 60s * 1000ms / ( dt_ms * 每转总计数 )
 * cpr = ENCODER_PPR * 4 * GEAR_RATIO
 *      ENCODER_PPR：编码器单圈脉冲数
 *      ×4：编码器4倍频计数模式
 *      GEAR_RATIO：电机减速比
 */
 uint16_t encoder_to_rpm(int32_t delta, uint32_t dt_ms)
{
	int32_t cpr;
	int64_t rpm;
	
	if(dt_ms == 0) return 0;
	
	cpr = ENCODER_PPR * 4 * GEAR_RATIO;
	if(cpr <= 0) return 0;
	
	rpm = ((int64_t)delta * 60000) / ((int64_t)dt_ms * cpr);
	if(rpm < 0) rpm = -rpm;
	if(rpm > 65535) rpm = 65535;
	
	return (uint16_t)rpm;
}

