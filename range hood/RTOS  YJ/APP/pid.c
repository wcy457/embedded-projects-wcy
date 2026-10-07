#include "pid.h"

void pid_init(pid_t *pid, float kp, float ki, float kd,int32_t out_min, int32_t out_max)
{
	pid->kd = kd;
	pid->ki = ki;
	pid->kp = kp;
	pid->integral = 0.0f;
	pid->last_error = 0;
	pid->out_min = out_min;
	pid->out_max = out_max;
}

/*
 * 位置式 PID公式：u(k) = Kp*e(k) + Ki*Σe + Kd*(e(k)-e(k-1))
 * 带积分限幅（抗积分饱和）与最终输出限幅。
 */
int32_t pid_update(pid_t *pid, int32_t setpoint, int32_t measurement)
{
	int32_t error = setpoint - measurement;   /* 计算当前偏差：目标值-实际反馈值 */
	float p,d;
	int32_t out;
	
	pid->integral += (float)error;
	/* 积分限幅，防止积分饱和 */
	if(pid->integral > (float)pid->out_max) pid->integral = (float)pid->out_max;
	if(pid->integral < (float)pid->out_min) pid->integral = (float)pid->out_min;
	
	p = pid->kp * (float)error;
	d = pid->kd * (float)(error - pid->last_error);
	
	out = (int32_t)(p + pid->ki * pid->integral + d);
	
	/* 输出限幅 */
	if (out > pid->out_max) out = pid->out_max;
  if (out < pid->out_min) out = pid->out_min;
	
	pid->last_error = error;/* 保存本次偏差，供下一次微分使用 */
	return out;
}
