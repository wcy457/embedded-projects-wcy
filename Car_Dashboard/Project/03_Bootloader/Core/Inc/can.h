/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    can.h
  * @brief   This file contains all the function prototypes for
  *          the can.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CAN_H__
#define __CAN_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

#define CAN_ID_CALL_OTA      0x0B3   /* OTA call / end summon: usb_tool -> Bootloader */
#define CAN_ID_CALL_OTA_ACK  0x0D3   /* OTA 应Ӧ�𣨵�15���Ѽӣ� */
#define CAN_ID_OTA_SEND      0x0B2   /* ISO-TP �̼����ݣ�31usb_tool��Bootloader�� */
#define CAN_ID_OTA_RECV      0x0B1   /* ISO-TP FC/ACK��Bootloader��31usb_tool�� */

/* USER CODE END Includes */

extern CAN_HandleTypeDef hcan;

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_CAN_Init(void);

/* USER CODE BEGIN Prototypes */

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __CAN_H__ */

