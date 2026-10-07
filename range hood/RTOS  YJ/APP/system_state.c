#include "system_state.h"
#include "FREERTOS.h"
#include "semphr.h"

system_state_t g_state;

static SemaphoreHandle_t s_mutex = NULL;

/**
 * @brief 系统状态初始化函数
 * @note 必须在FreeRTOS调度器启动(osKernelStart)之前调用
 */
void state_init(void)
{
	s_mutex = xSemaphoreCreateMutex();
	g_state.mode = MODE_OFF;
	g_state.gear = GEAR_LOW;
	g_state.target_rpm = 0;
	g_state.pwm_duty = 0;
	g_state.temperature = 0;
	g_state.humidity = 0;
	g_state.gas_ppm = 0;
	g_state.cooking_event = 0;
	g_state.backflow_active = 0;
}

/**
 * @brief 获取系统状态互斥锁
 * @details 在读写g_state全局变量前调用；获取锁之后别的任务无法修改该状态
 */
void state_lock(void)
{
	if(s_mutex != NULL){
		xSemaphoreTake(s_mutex, portMAX_DELAY);
	}
}

/**
 * @brief 释放系统状态互斥锁
 * @details g_state读写完成后必须解锁，否则其他任务将永久阻塞
 */
void state_unlock(void)
{
    if (s_mutex != NULL) {
        xSemaphoreGive(s_mutex);
    }
}
