/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "can.h"
#include "rtc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <stdio.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "lvgl/lvgl.h"
#include "lv_port_disp.h"
#include "lcd.h"
#include "ui/ui.h"
#include "canif.h"
#include "can_handle.h"
#include "boot_manager.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	
	 SCB->VTOR = 0x08008000;   /* APP 从 0x08008000 运行，向量表必须跟着挪 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_CAN_Init();
  Can_Filter_Init();              /* 配置CAN过滤器 + 启动CAN */
  MX_RTC_Init();
  MX_SPI1_Init();
  MX_TIM6_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	
	HAL_TIM_Base_Start_IT(&htim6);   /* 启动 TIM6 中断 → 开始喂 lv_tick */
  LCD_Init();                       /* 第 6 步的 LCD 初始化 */
  lv_init();                        /* LVGL 库初始化 (必须先于 lv_port_disp_init) */
  lv_port_disp_init();              /* 注册显示驱动 */

  ui_init();                       /* 建界面 */
	ui_set_rpm_angle(60);            /* 指针应指 150° */
	ui_set_safty_belt_color(1);      /* 安全带图标红 */
	ui_set_water_temp_color(1);      /* 水温图标红 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    static uint32_t lvtick = 0;
    static uint32_t rtc_tick = 0;
    uint32_t v;
		
		/* ===== OTA 触发检测（必须在 LVGL 之前） ===== */
			if (get_ota_flag() == 1) {
					inter_flash_cfg_set_app_update_flag(1);   /* 写 Flash 升级标志 */
					printf("[OTA] Restarting to OTA...\r\n");
					HAL_Delay(500);                            /* 等 printf 刷完 */
					HAL_NVIC_SystemReset();                    /* 复位进 Bootloader */
			}

		/* 1. LVGL 心跳节拍：每 5ms 调一次 */
    if (HAL_GetTick() - lvtick > 5) { lvtick = HAL_GetTick(); lv_timer_handler(); }

    /* 2. RTC 时间：每 500ms 读 RTC → sprintf → ui_set_date_time_value() */
    /*    注意：必须先 GetTime 再 GetDate（HAL 的 RTC 库要求） */
    if (HAL_GetTick() - rtc_tick > 500) {
        rtc_tick = HAL_GetTick();
        RTC_TimeTypeDef sTime = {0};
        RTC_DateTypeDef sDate = {0};
        HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
        char date_str[16];
        char time_str[16];
        sprintf(date_str, "20%02d-%02d-%02d", sDate.Year, sDate.Month, sDate.Date);
        sprintf(time_str, "%02d:%02d:%02d", sTime.Hours, sTime.Minutes, sTime.Seconds);
        ui_set_date_time_value((uint8_t *)date_str, (uint8_t *)time_str);
    }

    /* 3. CAN 收帧解析（生产者） */
    Can_Handle();

    /* 4. 有新数据就刷 UI（消费者）： */
    if (ui_get_rpm_value(&v) == 0)        ui_set_rpm_angle(v);
    if (ui_get_water_temp_value(&v) == 0) ui_set_water_temp_color(v);
    if (ui_get_safty_belt_value(&v) == 0) ui_set_safty_belt_color(v);
    if (ui_get_light_value(&v) == 0)      ui_set_light_color(v);
    if (ui_get_turn_light_value(&v) == 0) ui_set_turn_light_color(v);

    /* 5. CAN 收到 TYPE=06/07 的日期时间 → 写入 RTC */
    if (ui_get_date_value(&v) == 0) {
        RTC_DateTypeDef sDate = {0};
        sDate.Year  = (v >> 24) & 0xFF;
        sDate.Month = (v >> 16) & 0xFF;
        sDate.Date  = (v >> 8)  & 0xFF;
        HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    }
    if (ui_get_time_value(&v) == 0) {
        RTC_TimeTypeDef sTime = {0};
        sTime.Hours   = (v >> 24) & 0xFF;
        sTime.Minutes = (v >> 16) & 0xFF;
        sTime.Seconds = (v >> 8)  & 0xFF;
        HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM6) {
        lv_tick_inc(1);    /* ���� LVGL: ��ȥ�� 1ms */
    }
}
/* USER CODE END 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
