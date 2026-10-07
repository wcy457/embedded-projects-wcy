#ifndef __UART_DMA_H
#define __UART_DMA_H

#include "stm32f1xx_hal.h"

// ������ջ�������С�����UART_RX_BUF_SIZEδ����
#ifndef UART_RX_BUF_SIZE
#define UART_RX_BUF_SIZE        256U
#endif

// �ⲿ��������1��������huart1δ����
extern UART_HandleTypeDef huart1;

void uart_dma_init(void);
void uart_dma_idle_handler(void);
uint16_t uart_dma_read(uint8_t *out, uint16_t maxlen);

#endif
