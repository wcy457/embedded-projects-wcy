#ifndef __OLED_H
#define __OLED_H

#include "main.h"

/* 0.96 寸 SSD1306，I2C，128x64，8 页（每页 8 像素高） */

void oled_init(void);
void oled_clear(void);                                   /* 清整屏 */
void oled_clear_page(uint8_t page);                      /* 只清某一页（局部刷新用） */
void oled_draw_string(uint8_t page, uint8_t col, const char *str);
void oled_draw_int(uint8_t page, uint8_t col, int32_t val);

#endif /* __OLED_H */
