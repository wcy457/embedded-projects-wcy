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
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include <stdio.h>
#include <string.h>
#include "canif.h"
#include "inter_flashif.h"
#include "boot_manager.h"
#include "iso_to.h"

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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
	
	 Can_Filter_Init();                 /* ������ȫ���� + HAL_CAN_Start */

  printf("\r\n==== 34Bootloader start ====\r\n");

  uint8_t ota_flag = inter_flash_cfg_get_ota_flag();

	/* 首次上电参数区全是0xFF，自动写入一份出厂参数(flag=0)，省去手动烧录 */
	if (ota_flag == 0xFF) {
			printf("param area invalid, write factory default\r\n");
			inter_flash_cfg_set_app_update_flag(0);  // 将默认参数写入Flash参数区，升级标记置0
			ota_flag = 0;                            // 更新本地变量，标记为正常启动模式
	}
	printf("ota_flag = %d\r\n", ota_flag);

	if (ota_flag == 1) {
		printf("[OTA] entered upgrade mode\r\n");
		/* 发送 OTA ACK应答帧，通知上位机31usb_tool可以开始传输固件数据 */
		uint8_t ack_data[8] = {0xBE, 0xAD, 0xBE, 0xEF, 0x01, 0x00, 0x00, 0x00}; // OTA应答报文的8字节CAN数据
		Can_Send_Msg(CAN_ID_CALL_OTA_ACK, 8, ack_data);  // 调用CAN发送函数，发送ACK报文，ID为0x0D3，数据长度8字节
		printf("[OTA] ACK sent (0x0D3), waiting firmware ...\r\n");

		/* ISO-TP stack: poll in main loop, reassemble pages, callback writes Flash */
		iso_tp_init();
		while (1) {
				iso_tp_server();

				/* BEADBEEF+02 received: whole firmware transfer finished */
				if (iso_tp_is_ota_finished()) {
						printf("[OTA] all firmware received, clear ota_flag\r\n");
						inter_flash_cfg_set_app_update_flag(0);
						HAL_Delay(50);
						/* check APP image then jump; never returns on success */
						if (boot_check_stack2jump_app(INTER_FLASH_APP_ADDR) != 0) {
								printf("[OTA] invalid APP image, system halt\r\n");
								while (1) {
										HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
										HAL_Delay(200);
								}
						}
				}
		}
	}

/* flag=0：正常启动流程，准备跳转到APP应用程序 */
printf("jump to APP @0x%08X ...\r\n", INTER_FLASH_APP_ADDR);
// 校验APP镜像合法性：检查栈顶地址、复位向量是否合法
if (boot_check_stack2jump_app(INTER_FLASH_APP_ADDR) != 0) {
    printf("ERROR: no valid APP! LED fast blink\r\n");
    while (1) {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);   /* LED快闪200ms：APP不存在/镜像损坏校验失败 */
        HAL_Delay(200);
    }
}


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
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
}

/* USER CODE BEGIN 4 */

/* OTA application layer: 0xBF + offset(4, big-endian) + size(4, big-endian) + data */
#define OTA_PAGE_MAGIC    0xBFu
#define OTA_HEADER_LEN    9u

/* 4-byte aligned staging buffer for WORD programming (rx_buf payload starts at
 * an unaligned offset, uint32_t pointer casts must not be used on it directly) */
static uint32_t ota_fw_buf[FLASH_FAGE_SIZE / 4u];

/**
  * @brief ISO-TP upper layer callback: one complete OTA page reassembled.
  *        Big-endian parse -> range/alignment check -> erase -> write -> page ACK.
  * @param data pointer to full payload (ISO-TP buffer, not 4-byte aligned)
  * @param len  total payload length = 9 + firmware size
  */
void iso_tp_on_receive(uint8_t *data, uint16_t len)
{
    uint8_t  ack[8] = {0};
    uint32_t offset;
    uint32_t size;
    uint32_t dst;

    if (len < OTA_HEADER_LEN || data[0] != OTA_PAGE_MAGIC) {
        printf("[OTA] bad page magic/len\r\n");
        return;   /* no ACK -> sender times out and aborts the page */
    }

    /* big-endian, CPU-independent parsing */
    offset = ((uint32_t)data[1] << 24) | ((uint32_t)data[2] << 16) |
             ((uint32_t)data[3] <<  8) | ((uint32_t)data[4]);
    size   = ((uint32_t)data[5] << 24) | ((uint32_t)data[6] << 16) |
             ((uint32_t)data[7] <<  8) | ((uint32_t)data[8]);

    /* sanity: non-empty, WORD aligned, page aligned offset, inside APP region */
    if (size == 0u || (size & 0x03u) != 0u ||
        (offset % FLASH_FAGE_SIZE) != 0u ||
        offset > (INTER_FLASH_APP_END - INTER_FLASH_APP_ADDR) ||
        size   > (INTER_FLASH_APP_END - INTER_FLASH_APP_ADDR) - offset) {
        printf("[OTA] invalid offset=0x%08lX size=%lu\r\n", offset, size);
        return;
    }
    if ((uint32_t)(len - OTA_HEADER_LEN) < size) {
        printf("[OTA] declared size %lu > payload %u\r\n", size, len - OTA_HEADER_LEN);
        return;
    }

    dst = INTER_FLASH_APP_ADDR + offset;
    memcpy(ota_fw_buf, &data[OTA_HEADER_LEN], size);   /* align for WORD access */

    printf("[OTA] page @0x%08lX size=%lu erasing...\r\n", dst, size);
    if (inter_flashif_erase_page(dst) != 0u) {
        printf("[OTA] erase FAIL\r\n");
        return;
    }
    if (inter_flashif_write_page(dst, ota_fw_buf, size / 4u) != 0u) {
        printf("[OTA] write FAIL\r\n");
        return;
    }

    /* custom page ACK (byte0=0xBF) -> unblock sender wait_flow_control() */
    ack[0] = OTA_PAGE_MAGIC;
    Can_Send_Msg(CAN_ID_OTA_RECV, 8, ack);
    printf("[OTA] page OK, ACK sent\r\n");
}

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
