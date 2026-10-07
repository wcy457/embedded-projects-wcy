#ifndef __PID_H
#define __PID_H
#include <stdint.h>

typedef struct{
	float kp,ki,kd;/* PID三个控制参数：比例系数、积分系数、微分系数 */
	float integral;/* 积分累加值 */
	int32_t last_error;/* 上一次的偏差值，用于微分计算 */
	int32_t out_min,out_max;/* 输出限幅的最小值、最大值 */
}pid_t;

void     pid_init(pid_t *pid, float kp, float ki, float kd, int32_t out_min, int32_t out_max);
int32_t  pid_update(pid_t *pid, int32_t setpoint, int32_t measurement);

#endif
