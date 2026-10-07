#ifndef __BACKFLOW_H
#define __BACKFLOW_H

#include <stdint.h>

#define BACKFLOW_DUTY  800

/*
 * 防回流动态阈值（滞回比较器）：
 * 未激活时用正常阈值触发，激活后用较低防抖阈值关闭，避免临界抖动。
 * 返回 1 = 风机应启动，0 = 应关闭。
 */
void     backflow_init(void);
uint8_t  backflow_update(uint16_t gas_ppm);

#endif
