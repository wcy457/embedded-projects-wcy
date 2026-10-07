#ifndef __YMODEM_H
#define __YMODEM_H
#include "main.h"

#define PACKET_SIZE     128       // Ymodem SOH包：单包有效数据128字节
#define FLASH_PAGE_SIZE 2048     // STM32F103ZE Flash页大小：2KB


int32_t Ymodem_Receive(void);
#endif
