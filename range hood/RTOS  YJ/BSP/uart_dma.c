/* OTA 固件下载的数据接收通道 */

#include "uart_dma.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include <string.h>


static uint8_t s_rx[UART_RX_BUF_SIZE];
static volatile uint16_t s_rx_len = 0;
static SemaphoreHandle_t s_rx_sem = NULL;

/**
 * @brief  串口DMA接收初始化函数
 * @note   在main或者ota任务启动时调用一次
 */
void uart_dma_init(void)
{
	s_rx_sem = xSemaphoreCreateCounting(4, 0);// 最多缓存4次“收到数据包”的通知
	__HAL_UART_ENABLE_IT(&huart1, UART_IT_IDLE);// 开启串口空闲中断
	HAL_UART_Receive_DMA(&huart1, s_rx, UART_RX_BUF_SIZE);
}

/**
 * @brief 串口空闲中断处理函数
 * @note 必须放在USART1_IRQHandler()里面调用
 * 一包数据传输完成后进入此函数
 */
void uart_dma_idle_handler(void)
{
	// xHigherPriorityTaskWoken：任务调度标记
  // 如果唤醒了更高优先级任务，就需要进行一次任务切换
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_IDLE))
	{
		__HAL_UART_CLEAR_IDLEFLAG(&huart1);
		
		s_rx_len = UART_RX_BUF_SIZE - __HAL_DMA_GET_COUNTER(huart1.hdmarx);
		
		HAL_UART_AbortReceive(&huart1);
		HAL_UART_Receive_DMA(&huart1, s_rx, UART_RX_BUF_SIZE);
		
		if(s_rx_len > 0)
		{
			// 在中断里面释放信号量，通知OTA任务：有一包固件来了
       xSemaphoreGiveFromISR(s_rx_sem, &xHigherPriorityTaskWoken);
      // 如果唤醒高优先级任务，则立即切换任务
       portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
	}
}

/**
 * @brief 任务层读取串口数据包接口，给OTA升级任务调用
 * @param out: 上层任务的缓存，用来存放读到的固件数据包
 * @param maxlen: out缓冲区最大容量
 * @retval 实际读到的字节数
 */
uint16_t uart_dma_read(uint8_t *out, uint16_t maxlen)
{
	uint16_t n;
	
	if(xSemaphoreTake(s_rx_sem, portMAX_DELAY) != pdTRUE)
	{
		return 0;
	}
	
	n = s_rx_len;
	
	if(n > maxlen) n = maxlen;
	memcpy(out, s_rx, n);// 把DMA缓冲区s_rx中的固件数据拷贝到OTA任务的缓冲区out
	
	// 返回实际收到字节数，OTA任务拿到数据之后进行CRC校验、写入Flash
    return n;
}
