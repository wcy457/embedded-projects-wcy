#ifndef __FUSION_H
#define __FUSION_H

#include <stdint.h>

/* 气体浓度超过该阈值判定产生烹饪事件，单位 ppm */
#define COOKING_GAS_THRESHOLD 150

uint16_t fusion_compute(int16_t temp_x10, int16_t humid_x10, uint16_t gas_ppm);

#endif
