#ifndef __CANIF_H
#define __CANIF_H

#include "stm32f1xx_hal.h"

#define CAN_ID_CALL_OTA      0x0B3
#define CAN_ID_CALL_OTA_ACK  0x0D3
#define CAN_ID_OTA_SEND      0x0B2   /* ISO-TP ¹Ì¼þÊý¾Ý£¨31usb_tool¡úBootloader£© */
#define CAN_ID_OTA_RECV      0x0B1   /* ISO-TP FC/ACK£¨Bootloader¡ú31usb_tool£© */

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