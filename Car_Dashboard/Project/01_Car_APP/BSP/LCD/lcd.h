#ifndef __LCD_H
#define __LCD_H

#include "main.h"
#include "spi.h"

#define LCD_RES_PIN    GPIO_PIN_1    // LCD复位引脚 PA1
#define LCD_DC_PIN     GPIO_PIN_3    // 数据/命令选择引脚 PA3  0=命令，1=数据
#define LCD_CS_PIN     GPIO_PIN_4    // SPI片选引脚 PA4
#define LCD_BLK_PIN    GPIO_PIN_0    // LCD背光控制引脚 PA0

#define LCD_PORT       GPIOA

#define LCD_SCL_PIN     GPIO_PIN_5    // SPI1_SCK  PA5 时钟线
#define LCD_SDA_PIN     GPIO_PIN_7    // SPI1_MOSI PA7 主机输出数据线

// 引脚操作宏定义
#define LCD_RES(x)  HAL_GPIO_WritePin(LCD_PORT, LCD_RES_PIN, x)
#define LCD_DC(x)   HAL_GPIO_WritePin(LCD_PORT, LCD_DC_PIN, x)
#define LCD_CS(x)   HAL_GPIO_WritePin(LCD_PORT, LCD_CS_PIN, x)
#define LCD_BLK(x)  HAL_GPIO_WritePin(LCD_PORT, LCD_BLK_PIN, x)

extern SPI_HandleTypeDef hspi1;

#define LCD_WIDTH   480
#define LCD_HEIGHT  320

#define LCD_COLOR_WHITE   0xFFFF
#define LCD_COLOR_BLACK   0x0000
#define LCD_COLOR_RED     0xF800
#define LCD_COLOR_GREEN   0x07E0
#define LCD_COLOR_BLUE    0x001F



void  LCD_Init(void);
void  LCD_WriteCmd(uint8_t cmd);
void  LCD_WriteData(uint8_t data);
void  LCD_WriteData16(uint16_t data);
void  LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
void  LCD_Fill(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void  LCD_DrawPoint(uint16_t x, uint16_t y, uint16_t color);
void  LCD_ShowChar(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg, uint8_t size);
void  LCD_ShowString(uint16_t x, uint16_t y, char *str, uint16_t color, uint16_t bg, uint8_t size);

#endif /* __LCD_H */
