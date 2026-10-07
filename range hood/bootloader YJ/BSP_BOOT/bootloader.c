/*
 * @file  bootloader.c
 * @brief 独立裸机Bootloader，占用Flash前16KB 0x08000000~0x08003FFF
 *
 * 工作逻辑：
 * 1. 上电读取PB0按键：按住按键上电 → 进入串口升级；不按直接跳转APP
 * 2. 通信帧格式(小端LE)：[4字节payload长度][payload固件数据][4字节CRC32校验值]
 * 3. USART1 + DMA Normal模式分片接收，128字节每片，边接收边写入Flash
 * 4. 全部接收完成做CRC32校验；校验通过跳转APP，失败循环等待重传
 *
 * 注意事项：
 * 1. 本工程为裸机，禁止加入FreeRTOS；Keil IROM设置0x08000000，Size=0x4000
 * 2. USART1 RX-DMA 使用Normal模式，和APP工程Circular模式区分开
 * 3. NOTE: USART1 / DMA1_Ch4 / DMA1_Ch5 IRQ handlers in stm32f1xx_it.c MUST stay enabled
 * 4. 依赖crc32.c / crc32.h
 */
#include "bootloader.h"
#include "crc32.h"
#include "stm32f1xx_hal.h"
#include <string.h>

extern void SystemClock_Config(void);
extern void MX_GPIO_Init(void);
extern void MX_DMA_Init(void);
extern void MX_USART1_UART_Init(void);
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef  hdma_usart1_rx;
extern DMA_HandleTypeDef  hdma_usart1_tx;

/*==================== APP validity bounds (STM32F103ZET6, 64KB SRAM) ====================*/
#define RAM_BASE_ADDR          0x20000000UL
#define RAM_END_ADDR           0x20010000UL    /* SRAM exclusive end */
#define APP_FLASH_END_ADDR     (APP_BASE_ADDR + APP_MAX_SIZE)

/*==================== block receive timeouts (ms) ====================*/
#define BOOT_HEAD_TIMEOUT_MS   2000U
#define BOOT_DATA_TIMEOUT_MS   1000U
#define BOOT_TAIL_TIMEOUT_MS   1000U

void Error_Handler(void);

/*==================== Flash字写入内部状态变量 ====================*/
static uint32_t s_flash_write_addr;  /* 当前Flash写入地址 */
static uint32_t s_word_buf;          /* 4字节字缓存，F1 Flash只支持按字写入 */
static uint8_t  s_byte_cnt;          /* 当前缓存已经收到的字节计数 */

/**
 * @brief  Flash写缓冲复位，准备开始写入APP区域
 */
static void flash_write_state_reset(void)
{
	s_flash_write_addr = APP_BASE_ADDR;
	s_word_buf = 0U;
	s_byte_cnt = 0U;
}

/**
 * @brief  单字节写入Flash缓存，攒满4字节执行一次Word编程
 * @param  b: 需要写入的字节
 */
static void flash_write_byte(uint8_t b)
{
	/* 字节按位移入32位缓存，小端顺序 */
	s_word_buf |=((uint32_t)b) << (s_byte_cnt * 8U);
	s_byte_cnt ++;
	
	if(s_byte_cnt == 4U)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, s_flash_write_addr, s_word_buf) != HAL_OK)
		{
			Error_Handler();
		}
		s_flash_write_addr += 4U;
		s_word_buf = 0U;
    s_byte_cnt = 0U;
	}
}

/**
 * @brief  刷新写缓冲：收尾不足4字节时，补0xFF对齐，完成最后一次Flash写入
 */
static void flash_write_flush(void)
{
	while(s_byte_cnt > 0U && s_byte_cnt < 4U)
		{
			s_word_buf |= 0xFFU << (s_byte_cnt * 8U);
			s_byte_cnt++;
		}
	if(s_byte_cnt == 4U)
		{
			if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, s_flash_write_addr, s_word_buf) != HAL_OK)
			{
				Error_Handler();
			}
			s_flash_write_addr += 4U;
			s_word_buf = 0U;
			s_byte_cnt = 0U;
		}

}

/**
 * @brief  擦除全部APP所在Flash区域
 * @note   F1按页擦除，每页2KB；擦除时间几百ms，升级过程严禁断电
 */
static uint8_t flash_erase_all_app_page(void)
{
	FLASH_EraseInitTypeDef erase_cfg;/* HAL库Flash擦除配置结构体 */
	uint32_t page_error = 0U;
	uint32_t cur_addr = APP_BASE_ADDR;
	
	/* 循环，一页一页擦除，直到APP最大空间全部擦完 */
	while(cur_addr < (APP_BASE_ADDR + APP_MAX_SIZE))
	{
		erase_cfg.TypeErase    = FLASH_TYPEERASE_PAGES;   /* 模式：按页擦除 */
		erase_cfg.PageAddress = cur_addr;                 /* 当前要擦除的页起始地址 */
		erase_cfg.NbPages     = 1U;                      /* 每次只擦1页 */
		if(HAL_FLASHEx_Erase(&erase_cfg, &page_error) != HAL_OK)
		{
			return 0U;
		}
		cur_addr += F1_FLASH_PAGE_SIZE;                   /* 地址跳到下一页 */
	}
	return 1U;
}

/**
 * @brief Receive exactly n bytes via UART DMA (Normal mode) with timeout
 * @param dst: receive buffer
 * @param n: bytes to receive
 * @param timeout_ms: timeout in milliseconds
 * @retval 1 all n bytes received, 0 timeout or DMA/UART error (e.g. ORE)
 */
static uint8_t uart_dma_block_recv(uint8_t *dst, uint16_t n, uint32_t timeout_ms)
{
	uint32_t tickstart;

	if(HAL_UART_Receive_DMA(&huart1, dst, n) != HAL_OK)
	{
		return 0U;
	}

	tickstart = HAL_GetTick();
	while(huart1.RxState != HAL_UART_STATE_READY)
	{
		if((HAL_GetTick() - tickstart) > timeout_ms)
		{
			(void)HAL_UART_AbortReceive(&huart1);
			return 0U;
		}
	}

	/* ORE/FE can make HAL abort early (READY) before all bytes arrive */
	if((huart1.hdmarx != NULL) && (__HAL_DMA_GET_COUNTER(huart1.hdmarx) != 0U))
	{
		return 0U;
	}
	return 1U;
}

/**
 * @brief 判断升级按键PB0是否按下
 * @retval 1：按键按下；0：按键松开
 * @note CubeMX配置PB0为上拉输入；按键按下引脚拉低，为GPIO_PIN_RESET
 */
uint8_t boot_is_key_pressed(void)
{
    if(HAL_GPIO_ReadPin(BOOT_KEY_GPIO_PORT, BOOT_KEY_GPIO_PIN) == GPIO_PIN_RESET)
    {
        return 1U;
    }
    return 0U;
}

/**
 * @brief Validate the candidate APP image
 * @param msp: first word of APP image (initial MSP)
 * @param entry: second word of APP image (reset handler, bit0=1 Thumb)
 * @retval 1 valid, 0 invalid (blank chip / corrupted header)
 */
static uint8_t boot_is_app_valid(uint32_t msp, uint32_t entry)
{
	if((msp < RAM_BASE_ADDR) || (msp >= RAM_END_ADDR))
	{
		return 0U;
	}
	if((entry < APP_BASE_ADDR) || (entry >= APP_FLASH_END_ADDR))
	{
		return 0U;
	}
	if((entry & 1U) == 0U)
	{
		return 0U;
	}
	return 1U;
}

/**
 * @brief DeInit peripherals and jump to APP reset handler
 * @retval 0 APP invalid (caller stays in update mode); never returns on success
 */
uint8_t boot_jump_to_app(void)
{
	uint32_t app_msp_stack;
	uint32_t app_reset_entry;
	uint32_t idx;

	app_msp_stack = *(volatile uint32_t *)APP_BASE_ADDR;
	app_reset_entry = *(volatile uint32_t *)(APP_BASE_ADDR + 4U);

	if(boot_is_app_valid(app_msp_stack, app_reset_entry) == 0U)
	{
		return 0U;
	}

	/* 1. Stop bootloader SysTick before handing over */
	SysTick->CTRL = 0U;
	SysTick->LOAD = 0U;
	SysTick->VAL  = 0U;

	/* 2. Disable and clear every NVIC IRQ left enabled by bootloader */
	for(idx = 0U; idx < 8U; idx++)
	{
		NVIC->ICER[idx] = 0xFFFFFFFFU;
		NVIC->ICPR[idx] = 0xFFFFFFFFU;
	}

	/* 3. DeInit UART/DMA so APP re-initializes them from a clean state */
	HAL_UART_DeInit(&huart1);
	HAL_DMA_DeInit(&hdma_usart1_rx);
	HAL_DMA_DeInit(&hdma_usart1_tx);

	/* 4. Point VTOR at APP vector table (APP SystemInit sets it again) */
	SCB->VTOR = APP_BASE_ADDR;

	/* 5. Mask interrupts, switch MSP, branch to APP reset handler */
	__disable_irq();
	__set_MSP(app_msp_stack);
	((void (*)(void))app_reset_entry)();

	return 1U;
}

/**
 * @brief Bootloader升级主流程
 * @note 循环等待升级帧；接收-写Flash-CRC校验；校验成功跳转APP，失败继续等待重传
 */
void boot_run_update(void)
{
	uint32_t payload_total_len;     /* 固件payload总长度，来自帧头4字节小端 */
	uint32_t crc_temp;              /* CRC迭代计算中间变量 */
	uint32_t crc_calculate_result;  /* 本地计算完成后的CRC结果 */
	uint32_t crc_from_uart;         /* 上位机下发的CRC校验值 */
	uint32_t remain_byte;            /* 还剩余多少字节未接收 */
	uint16_t recv_chunk_len;         /* 当前分片接收字节数 */
	uint32_t i;
	uint8_t recv_buf[BOOT_CHUNK_SIZE];
	uint8_t recv_ok;                     /* data stage receive status */
  
	for(;;)
	{
		/* ========== 第一步：接收4字节帧头，payload长度(小端) ========== */
		if(uart_dma_block_recv((uint8_t *)&payload_total_len, 4U, BOOT_HEAD_TIMEOUT_MS) == 0U)
		{
			continue;
		}
		/* 合法性检查：长度等于0，或者超过APP最大可用Flash空间，丢弃本帧 */
		if((payload_total_len == 0U) || (payload_total_len > (APP_MAX_SIZE - 4U)))
		{
				continue;
		}
		
		/* ========== 第二步：解锁Flash，擦除APP全部页面，初始化写状态 ========== */
		HAL_FLASH_Unlock();
		if(flash_erase_all_app_page() == 0U)
		{
			uint8_t nack = 'N';   /* tell host: erase failed, resend whole frame */
			HAL_UART_Transmit(&huart1, &nack, 1U, 100U);
			HAL_FLASH_Lock();
			continue;
		}
		flash_write_state_reset();         /* 复位Flash写缓冲 */
		crc_temp = 0xFFFFFFFFU;
		{
			uint8_t ready = 'R';  /* erase done, host may stream data now */
			HAL_UART_Transmit(&huart1, &ready, 1U, 100U);
		}                          /* CRC32初始值固定0xFFFFFFFF */
		
		/* ========== 第三步：循环分片接收固件数据，边写Flash边计算CRC ========== */
		remain_byte = payload_total_len;
		recv_ok = 1U;
		while(remain_byte > 0U)
		{
			/* 本次接收长度：剩余大于缓冲区就取缓冲区大小，否则取剩余字节 */
			recv_chunk_len = (remain_byte > BOOT_CHUNK_SIZE) ? BOOT_CHUNK_SIZE : (uint16_t)remain_byte;
			if(uart_dma_block_recv(recv_buf, recv_chunk_len, BOOT_DATA_TIMEOUT_MS) == 0U)
			{
				recv_ok = 0U;
				break;
			}

			/* 逐个字节送入Flash缓存 */
			for(i = 0U; i < recv_chunk_len; i++)
			{
					flash_write_byte(recv_buf[i]);
			}
			/* 更新CRC32校验值 */
			crc_temp = crc32_update(crc_temp, recv_buf, recv_chunk_len);
			remain_byte -= recv_chunk_len;
		}
		
		if(recv_ok == 0U)
		{
			uint8_t nack = 'N';   /* tell host: data stage failed, resend whole frame */
			HAL_UART_Transmit(&huart1, &nack, 1U, 100U);
			HAL_FLASH_Lock();
			continue;
		}

		flash_write_flush();    /* 处理最后不足4字节余数 */
		HAL_FLASH_Lock();       /* Flash上锁，禁止后续意外写操作 */

		crc_calculate_result = crc32_finalize(crc_temp);  /* CRC收尾取反，得到最终校验码 */

		/* ========== 第四步：接收上位机传来的4字节CRC校验值 ========== */
		if(uart_dma_block_recv((uint8_t *)&crc_from_uart, 4U, BOOT_TAIL_TIMEOUT_MS) == 0U)
		{
			continue;
		}

		/* ========== 第五步：校验对比，成功跳转APP，失败继续循环等待重传 ========== */
		if(crc_from_uart == crc_calculate_result)
		{
			uint8_t ack = 'K';    /* ACK: image verified, jump to APP */
			HAL_UART_Transmit(&huart1, &ack, 1U, 100U);
			(void)boot_jump_to_app();
		}
		else
		{
			uint8_t nack = 'N';   /* NAK: CRC mismatch, host resends */
			HAL_UART_Transmit(&huart1, &nack, 1U, 100U);
		}
	}
}

/**
 * @brief bootloader程序入口main
 * @retval boot裸机，正常不会return
 */
int main(void)
{
	HAL_Init();                     /* HAL库初始化，SysTick初始化 */
	SystemClock_Config();           /* 配置系统时钟72MHz */
	MX_GPIO_Init();                 /* GPIO初始化，包含PB0升级按键 */
	MX_DMA_Init();                  /* DMA控制器初始化 */
	MX_USART1_UART_Init();          /* USART1串口初始化，DMA?Normal模式 */

	/* 判断按键：没有按下，直接跳转APP运行 */
	if(!boot_is_key_pressed())
	{
			(void)boot_jump_to_app();
	}

	/* 按键按下，进入固件升级模式 */
	boot_run_update();

	/* 理论不会执行到此处 */
	while(1)
	{
			;
	}
}

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
