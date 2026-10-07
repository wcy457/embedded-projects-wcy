#include "fusion.h"
#include "app_config.h"

/*
 * 融合算法数学原型：
 * f_T = clamp((T - T_base) / RANGE_T, 0, 1)
 * f_H = clamp((H - H_base) / RANGE_H, 0, 1)
 * f_G = clamp((G - G_base) / RANGE_G, 0, 1)
 * F   = 0.2*f_T + 0.2*f_H + 0.6*f_G      权重分配：温度0.2，湿度0.2，气体0.6，权重总和为1
 * PWM = 1% + F * 99%
 *
 * 工程实现：整体放大1000倍转为定点整数运算（F1000 = F × 1000），消除浮点运算。
 * 逻辑说明：
 * 1、各传感器减去环境基线，只把超出基线的扰动作为有效输入；
 * 2、做min-max归一化钳位到0~1，消除不同传感器量纲差异；
 * 3、线性加权，气体油烟信号作为主判据，温湿度作为辅助补偿；
 * 4、映射输出PWM，最低保留基础占空比，不允许风机完全停机。
 */

/**
 * @brief 温湿度+气体多传感器融合计算目标PWM千分占空比
 * @param temp_x10    温度，放大10倍定点数值
 * @param humid_x10   湿度，放大10倍定点数值
 * @param gas_ppm     气体浓度，单位ppm
 * @return uint16_t   输出PWM千分占空比
 */
uint16_t fusion_compute(int16_t temp_x10, int16_t humid_x10, uint16_t gas_ppm)
{
	int32_t fT, fH, fG;
	int32_t F1000;
	int32_t pwm;
	
	/* 温度扰动归一化：减去环境基线，低于基线扰动置0，超过量程做上限钳位 */
	fT = (int32_t)temp_x10 - BASE_TEMP;
	if(fT < 0) fT = 0;
	if(fT > RANGE_TEMP) fT = RANGE_TEMP;
	
	/* 湿度扰动归一化：减去环境基线，低于基线扰动置0，超过量程做上限钳位 */
	fH = (int32_t)humid_x10 - BASE_HUMID;
	if (fH < 0) fH = 0;
	if (fH > RANGE_HUMID) fH = RANGE_HUMID;

	/* 气体浓度扰动归一化：减去环境基线，低于基线扰动置0，超过量程做上限钳位 */
	fG = (int32_t)gas_ppm - BASE_GAS;
	if (fG < 0) fG = 0;
	if (fG > RANGE_GAS) fG = RANGE_GAS;
	
	/*
     * 加权求和得到0~1000的定点融合系数F1000
     * FUSION_W_T/H/G 权重以0.1为单位；×100换算千分权重，全程整数运算
     * 除以各传感器量程，完成归一化加权
     */
	F1000 = (FUSION_W_T * fT) / RANGE_TEMP +
	        (FUSION_W_H * fH) / RANGE_HUMID +
					(FUSION_W_G * fG) / RANGE_GAS;
					
   /* 将0~1000融合系数线性映射到PWM输出区间：基础占空比 ~ 最大占空比 */
   pwm = PWM_BASE_PERMILLE + 
      (F1000 * (PWM_PERMILLE_MAX - PWM_BASE_PERMILLE)) / 1000;
			
  /* PWM输出限幅保护，防止参数异常导致输出越界 */
	if (pwm < PWM_BASE_PERMILLE) pwm = PWM_BASE_PERMILLE;
	if (pwm > PWM_PERMILLE_MAX)  pwm = PWM_PERMILLE_MAX;

	return (uint16_t)pwm;
}
