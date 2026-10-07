#ifndef __CANIF_H
#define __CANIF_H

#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif


void Can_Filter_Init(void);

uint32_t Can_Send_Msg(uint32_t id, uint32_t len, uint8_t *buf);

uint32_t Can_Recv_Msg(uint32_t *id, uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif
