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

#include "canif.h"

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

/* ========== UART 帧协议: AF + len + data[len] + FA, len = 2~5 ========== */
#define UART_FRAME_HEAD  0xAF
#define UART_FRAME_TAIL  0xFA

typedef struct {
    uint8_t rx_buf[16];    /* 接收缓冲区 */
    uint8_t rx_cnt;        /* 已收字节数 */
} uart_rx_t;

static uart_rx_t uart2_rx = {0};
static uint8_t   uart2_byte = 0;   /* 单字节 IT 接收的搬运格 */

/* ========== UART2 接收中断回调: 每收 1 字节进一次 ========== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        uart2_rx.rx_buf[uart2_rx.rx_cnt++] = uart2_byte;

        /* 1. 帧头校验: 首字节必须是 0xAF */
        if (uart2_rx.rx_buf[0] != UART_FRAME_HEAD) {
            uart2_rx.rx_cnt = 0;                       /* 丢弃, 重新等帧头 */
            printf("[ERR] head=0x%02X not AF\r\n", uart2_rx.rx_buf[0]);
        }
        /* 2. 防溢出 */
        else if (uart2_rx.rx_cnt >= sizeof(uart2_rx.rx_buf)) {
            uart2_rx.rx_cnt = 0;
            printf("[ERR] buffer overflow\r\n");
        }
        /* 3. 收满 3 字节后开始检查 len */
        else if (uart2_rx.rx_cnt > 2) {
            uint8_t data_len = uart2_rx.rx_buf[1];

            if (data_len < 2 || data_len > 5) {        /* len 必须 2~5 */
                uart2_rx.rx_cnt = 0;
                printf("[ERR] len=%d invalid\r\n", data_len);
            }
            /* 4. 收满: AF(1)+len(1)+data(len)+FA(1) = len+3 字节 */
            else if (uart2_rx.rx_cnt >= data_len + 3) {
                if (uart2_rx.rx_buf[data_len + 2] == UART_FRAME_TAIL) {
                    /* 组 CAN 帧: 8 字节, 首字节 AF, 后跟 data, 不足补 0 */
                    uint8_t can_tx[8] = {0};
                    can_tx[0] = 0xAF;
                    for (uint8_t i = 0; i < data_len; i++)
                        can_tx[i + 1] = uart2_rx.rx_buf[i + 2];

                    if (Can_Send_Msg(0x0B4, 8, can_tx) == 0) {
                        printf("[CAN] OK: ");
                        for (int i = 0; i < 8; i++) printf("%02X ", can_tx[i]);
                        printf("\r\n");
                    } else {
                        printf("[ERR] can send fail\r\n");
                    }
                } else {
                    printf("[ERR] tail=0x%02X\r\n", uart2_rx.rx_buf[data_len + 2]);
                }
                uart2_rx.rx_cnt = 0;                   /* 复位, 等下一帧 */
            }
        }
        /* 5. 重新挂接收, 忘了这行中断就一次性了 */
        HAL_UART_Receive_IT(&huart2, &uart2_byte, 1);
    }
}

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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	
  Can_Filter_Init();          /* CAN 过滤器 + 启动 */
  HAL_UART_Receive_IT(&huart2, &uart2_byte, 1);   /* 挂 UART2 中断接收 */
  printf("==== 29C8T6 Send OK ====\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_Delay(10);            /* 数据收发全在 UART2 中断里, 主循环空转 */
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
