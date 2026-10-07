#include "motor.h"

void motor_init(void)
{
	HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
	motor_set_dir(1);
	motor_set_duty(0);
	motor_enable(1);
}

/**
 * @brief  设置电机PWM输出占空比（千分比方式 0?1000）
 * @param  permille：千分比占空比，范围：0 ~ PWM_PERMILLE_MAX
 *         permille = 0    → 占空比 0%   电机停止
 *         permille = 500  → 占空比 50%
 *         permille = 1000 → 占空比 100% 满速运行
 * @retval 无
 * @note   PWM_PERMILLE_MAX 在motor.h中一般定义为1000
 */
void motor_set_duty(uint16_t permille)
{
	uint32_t arr, cmp;
	
	if(permille > PWM_PERMILLE_MAX) permille = PWM_PERMILLE_MAX;
	
	/* 读取定时器TIM1的自动重装载值ARR（PWM周期值） */
	arr = __HAL_TIM_GET_AUTORELOAD(&htim1);
	
	/**
     * 计算CCR比较值
     * cmp = 千分比 * (ARR+1) / 1000
     * ARR+1：定时器一个PWM周期内计数的总点数
     */
	cmp = (uint32_t)permille * (arr + 1) / PWM_PERMILLE_MAX;
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, cmp);
}

/**
 * @brief 设置电机转向
 * @param forward 方向选择
 *         1 → 正转：AIN1=高电平，AIN2=低电平
 *         0 → 反转：AIN1=低电平，AIN2=高电平
 * @note TB6612FNG方向控制逻辑
 */
void motor_set_dir(uint8_t forward)
{
    /* TB6612FNG：AIN1/AIN2 逻辑组合决定方向
     *   forward=1（正转）：AIN1=H，AIN2=L
     *   forward=0（反转）：AIN1=L，AIN2=H
     */
    HAL_GPIO_WritePin(MOTOR_AIN1_PORT, MOTOR_AIN1_PIN,
                      forward ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MOTOR_AIN2_PORT, MOTOR_AIN2_PIN,
                      forward ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
 * @brief TB6612FNG休眠/使能控制
 * @param on 1?解除休眠，芯片可以输出；0?STBY低电平，驱动休眠，电机滑行
 */
void motor_enable(uint8_t on)
{
    HAL_GPIO_WritePin(MOTOR_STBY_PORT, MOTOR_STBY_PIN, on ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

