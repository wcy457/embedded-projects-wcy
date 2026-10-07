#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

/*
 * 读取一次温湿度。成功返回 1，失败返回 0。
 * temp_x10 / humid_x10 为 ×10 的定点值（如 25.3℃ → 253）。
 * 注意：内部使用微秒级忙等，需在关中断或暂停调度器下调用，保证时序。
 */
uint8_t dht11_read(int16_t *temp_x10, int16_t *humid_x10);

#endif /* __DHT11_H */
