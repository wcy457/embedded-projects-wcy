/**
 * @file u8g2_bitmap.c
 * @brief u8g2库的位图绘制功能实现文件
 *
 * 本文件提供了在u8g2图形库中绘制位图的核心功能，包括：
 * - 设置位图绘制模式（透明/不透明）
 * - 绘制水平位图行
 * - 绘制完整的位图（标准格式和XBM格式）
 * - 支持从程序存储器（PROGMEM）读取位图数据
 *
 * 位图格式说明：
 * - 标准BMP格式：每个字节的最高位(MSB)对应最左边的像素
 * - XBM格式：每个字节的最低位(LSB)对应最左边的像素
 * - XBMP格式：与XBM相同，但数据存储在程序存储器(PROGMEM)中
 *
 * 适用于STM32智能手表项目的图标和图片显示功能。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 * Copyright (c) 2016, olikraus@gmail.com
 */

/*
  原始版权声明（BSD许可证）：

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list
    of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or other
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "u8g2.h"


/**
 * @brief 设置位图绘制的透明模式
 * @param u8g2        u8g2显示结构体指针
 * @param is_transparent 透明模式标志：0=不透明（背景会被填充），1=透明（只绘制前景像素）
 */
void u8g2_SetBitmapMode(u8g2_t *u8g2, uint8_t is_transparent) {
  u8g2->bitmap_transparency = is_transparent;  // 设置位图透明度属性
}

/**
 * @brief 绘制水平位图行（标准BMP格式，MSB在前）
 * @param u8g2  u8g2显示结构体指针
 * @param x     起始X坐标（像素）
 * @param y     起始Y坐标（像素）
 * @param len   位图行的长度（像素数）
 * @param b     指向位图数据的指针（每个字节的最高位对应最左边的像素）
 *
 * 逐像素绘制位图行，每个字节从最高位(MSB)开始解析。
 * 如果bitmap_transparency为0（不透明模式），未设置的像素也会用相反颜色绘制。
 */
void u8g2_DrawHorizontalBitmap(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, const uint8_t *b)
{
  uint8_t mask;                                   // 位掩码，用于逐位提取像素
  uint8_t color = u8g2->draw_color;               // 保存当前绘制颜色
  uint8_t ncolor = (color == 0 ? 1 : 0);          // 计算反色（用于不透明模式绘制背景）

#ifdef U8G2_WITH_INTERSECTION
  /* 检查绘制区域是否与屏幕可见区域相交，不相交则直接返回 */
  if ( u8g2_IsIntersection(u8g2, x, y, x+len, y+1) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  mask = 128;  // 从字节的最高位开始（MSB对应最左边的像素）
  while(len > 0)
  {
    if ( *b & mask ) {
      /* 当前位为1，用前景色绘制像素 */
      u8g2->draw_color = color;
      u8g2_DrawHVLine(u8g2, x, y, 1, 0);
    } else if ( u8g2->bitmap_transparency == 0 ) {
      /* 当前位为0且非透明模式，用背景色绘制像素 */
      u8g2->draw_color = ncolor;
      u8g2_DrawHVLine(u8g2, x, y, 1, 0);
    }

    x++;           // 移动到下一个像素位置
    mask >>= 1;    // 掩码右移，检查下一个位
    if ( mask == 0 )
    {
      mask = 128;  // 一个字节处理完毕，重置掩码
      b++;         // 指向下一个字节
    }
    len--;
  }
  u8g2->draw_color = color;  // 恢复原始绘制颜色
}


/**
 * @brief 绘制完整的位图（标准BMP格式，兼容u8glib）
 * @param u8g2    u8g2显示结构体指针
 * @param x       位图左上角X坐标
 * @param y       位图左上角Y坐标
 * @param cnt     每行的字节数（宽度 = cnt * 8 像素）
 * @param h       位图的高度（行数）
 * @param bitmap  指向位图数据的指针
 *
 * 此函数与u8glib兼容，通过逐行调用u8g2_DrawHorizontalBitmap实现完整位图绘制。
 */
void u8g2_DrawBitmap(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t cnt, u8g2_uint_t h, const uint8_t *bitmap)
{
  u8g2_uint_t w;
  w = cnt;       // 将字节数赋值给宽度变量
  w *= 8;        // 转换为像素宽度（1字节 = 8像素）
#ifdef U8G2_WITH_INTERSECTION
  /* 检查位图区域是否与屏幕可见区域相交 */
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  while( h > 0 )
  {
    u8g2_DrawHorizontalBitmap(u8g2, x, y, w, bitmap);  // 绘制一行位图
    bitmap += cnt;   // 指向下一行数据
    y++;             // Y坐标递增
    h--;             // 剩余行数递减
  }
}



/**
 * @brief 绘制水平位图行（XBM格式，LSB在前）
 * @param u8g2  u8g2显示结构体指针
 * @param x     起始X坐标（像素）
 * @param y     起始Y坐标（像素）
 * @param len   位图行的长度（像素数）
 * @param b     指向XBM位图数据的指针（每个字节的最低位对应最左边的像素）
 *
 * XBM格式与标准BMP格式的区别：XBM从最低位(LSB)开始解析，BMP从最高位(MSB)开始。
 */
void u8g2_DrawHXBM(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, const uint8_t *b)
{
  uint8_t mask;                                   // 位掩码
  uint8_t color = u8g2->draw_color;               // 保存当前绘制颜色
  uint8_t ncolor = (color == 0 ? 1 : 0);          // 计算反色
#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+len, y+1) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  mask = 1;  // XBM格式从最低位开始（LSB对应最左边的像素）
  while(len > 0) {
    if ( *b & mask ) {
      /* 当前位为1，用前景色绘制像素 */
      u8g2->draw_color = color;
      u8g2_DrawHVLine(u8g2, x, y, 1, 0);
    } else if ( u8g2->bitmap_transparency == 0 ) {
      /* 当前位为0且非透明模式，用背景色绘制像素 */
      u8g2->draw_color = ncolor;
      u8g2_DrawHVLine(u8g2, x, y, 1, 0);
    }
    x++;           // 移动到下一个像素位置
    mask <<= 1;    // 掩码左移，检查下一个位（XBM从低位到高位）
    if ( mask == 0 )
    {
      mask = 1;    // 一个字节处理完毕，重置掩码
      b++;         // 指向下一个字节
    }
    len--;
  }
  u8g2->draw_color = color;  // 恢复原始绘制颜色
}


/**
 * @brief 绘制完整的XBM格式位图
 * @param u8g2    u8g2显示结构体指针
 * @param x       位图左上角X坐标
 * @param y       位图左上角Y坐标
 * @param w       位图的宽度（像素）
 * @param h       位图的高度（行数）
 * @param bitmap  指向XBM位图数据的指针
 *
 * XBM格式常用于嵌入式系统中的图标资源，由X Window系统定义。
 */
void u8g2_DrawXBM(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h, const uint8_t *bitmap)
{
  u8g2_uint_t blen;
  blen = w;        // 获取宽度
  blen += 7;       // 加7用于向上取整
  blen >>= 3;      // 右移3位，等价于除以8，得到每行的字节数
#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  while( h > 0 )
  {
    u8g2_DrawHXBM(u8g2, x, y, w, bitmap);  // 绘制一行XBM位图
    bitmap += blen;   // 指向下一行数据（按字节对齐）
    y++;              // Y坐标递增
    h--;              // 剩余行数递减
  }
}






/**
 * @brief 绘制水平位图行（XBM格式，数据存储在程序存储器PROGMEM中）
 * @param u8g2  u8g2显示结构体指针
 * @param x     起始X坐标（像素）
 * @param y     起始Y坐标（像素）
 * @param len   位图行的长度（像素数）
 * @param b     指向PROGMEM中XBM位图数据的指针
 *
 * 与u8g2_DrawHXBM功能相同，但使用u8x8_pgm_read从程序存储器读取数据，
 * 适用于AVR等哈佛架构的微控制器。在STM32中通常直接从Flash读取。
 */
void u8g2_DrawHXBMP(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, const uint8_t *b)
{
  uint8_t mask;
  uint8_t color = u8g2->draw_color;
  uint8_t ncolor = (color == 0 ? 1 : 0);
#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+len, y+1) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  mask = 1;  // XBM格式从最低位开始
  while(len > 0)
  {
    if( u8x8_pgm_read(b) & mask ) {   // 使用pgm_read从程序存储器读取数据
      u8g2->draw_color = color;
      u8g2_DrawHVLine(u8g2, x, y, 1, 0);
    } else if( u8g2->bitmap_transparency == 0 ) {
      u8g2->draw_color = ncolor;
      u8g2_DrawHVLine(u8g2, x, y, 1, 0);
    }

    x++;
    mask <<= 1;
    if ( mask == 0 )
    {
      mask = 1;
      b++;
    }
    len--;
  }
  u8g2->draw_color = color;
}


/**
 * @brief 绘制完整的XBM格式位图（数据存储在程序存储器PROGMEM中）
 * @param u8g2    u8g2显示结构体指针
 * @param x       位图左上角X坐标
 * @param y       位图左上角Y坐标
 * @param w       位图的宽度（像素）
 * @param h       位图的高度（行数）
 * @param bitmap  指向PROGMEM中XBM位图数据的指针
 *
 * 适用于图标等存储在Flash中的位图资源。
 */
void u8g2_DrawXBMP(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h, const uint8_t *bitmap)
{
  u8g2_uint_t blen;
  blen = w;        // 获取宽度
  blen += 7;       // 加7用于向上取整
  blen >>= 3;      // 右移3位，得到每行的字节数
#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  while( h > 0 )
  {
    u8g2_DrawHXBMP(u8g2, x, y, w, bitmap);  // 绘制一行PROGMEM中的XBM位图
    bitmap += blen;   // 指向下一行数据
    y++;              // Y坐标递增
    h--;              // 剩余行数递减
  }
}


