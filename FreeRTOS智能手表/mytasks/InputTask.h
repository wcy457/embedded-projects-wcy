#ifndef __INPUT_TASK_H__
#define __INPUT_TASK_H__

#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief  输入任务：轮询红外遥控 + 旋转编码器，发送 Key_data 到队列
 * @param  params: 未使用
 */
void InputTask(void *params);

#endif
