/**
 * @file u8g2_ll_hvline.c
 * @brief u8g2库的底层水平/垂直线绘制功能实现文件
 *
 * 本文件实现了u8g2图形库中底层的水平线和垂直线绘制功能。
 * 针对不同的显示控制器，提供了两种像素布局方式的实现：
 * 1. vertical_top_lsb - 垂直字节布局，LSB在顶部（适用于SSD13xx、UC1701等）
 * 2. horizontal_right_lsb - 水平字节布局，LSB在右侧（适用于ST7920等）
 *
 * 像素操作原理：
 *   *ptr |= or_mask   // 设置像素位
 *   *ptr ^= xor_mask   // 翻转像素位
 *
 * 颜色与掩码的关系：
 *   color = 0（清除）: or_mask = 1, xor_mask = 1  -> 最终结果为0
 *   color = 1（设置）: or_mask = 1, xor_mask = 0  -> 最终结果为1
 *   color = 2（异或）: or_mask = 0, xor_mask = 1  -> 翻转当前值
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 *
 * Copyright (c) 2016, olikraus@gmail.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or other
 *   materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * 像素操作原理：
 *   *ptr |= or_mask    // 设置像素位
 *   *ptr ^= xor_mask   // 翻转像素位
 *
 * 颜色与掩码的关系：
 *   color = 0:   or_mask = 1, xor_mask = 1  -> 清除像素
 *   color = 1:   or_mask = 1, xor_mask = 0  -> 设置像素
 *   color = 2:   or_mask = 0, xor_mask = 1  -> 翻转像素（XOR）
 *
 */

#include "u8g2.h"
#include <assert.h>

/*=================================================*/
/**
 * @brief 垂直字节布局，LSB在顶部的线段绘制
 *
 * 适用于以下显示控制器：
 *   - SSD13xx系列（如SSD1306、SSD1309等）
 *   - UC1701
 *
 * 像素布局方式：
 *   每个字节表示垂直方向的8个像素
 *   字节的bit0（LSB）对应顶部像素（y=0）
 *   字节的bit7（MSB）对应底部像素（y=7）
 */


#ifdef U8G2_WITH_HVLINE_SPEED_OPTIMIZATION

/**
 * @brief 垂直字节布局的线段绘制（速度优化版本）
 *
 * 优化版本直接操作内存，减少函数调用开销。
 * 适用于SSD13xx、UC1701等显示控制器。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标（本地缓冲区内）
 * @param y     线段起始点的y坐标（本地缓冲区内）
 * @param len   线段长度（像素），不能为0
 * @param dir   方向: 0=水平线（从左到右），1=垂直线（从上到下）
 *
 * @note 假设所有裁剪已完成
 */
void u8g2_ll_hvline_vertical_top_lsb(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  uint16_t offset;
  uint8_t *ptr;
  uint8_t bit_pos, mask;
  uint8_t or_mask, xor_mask;
#ifdef __unix
  uint8_t *max_ptr = u8g2->tile_buf_ptr + u8g2_GetU8x8(u8g2)->display_info->tile_width*u8g2->tile_buf_height*8;
#endif

  //assert(x >= u8g2->buf_x0);
  //assert(x < u8g2_GetU8x8(u8g2)->display_info->tile_width*8);
  //assert(y >= u8g2->buf_y0);
  //assert(y < u8g2_GetU8x8(u8g2)->display_info->tile_height*8);

  /* 计算位掩码：垂直字节布局，LSB在顶部 */
  bit_pos = y;          /* 获取y坐标（溢出截断在这里是安全的） */
  bit_pos &= 7;        /* 只需要低3位（0-7） */
  mask = 1;
  mask <<= bit_pos;     /* 将掩码移位到对应位置 */

  /* 根据绘图颜色设置or_mask和xor_mask */
  or_mask = 0;
  xor_mask = 0;
  if ( u8g2->draw_color <= 1 )
    or_mask  = mask;     /* 颜色0或1时使用or_mask */
  if ( u8g2->draw_color != 1 )
    xor_mask = mask;     /* 颜色0或2时使用xor_mask */

  /* 计算像素在缓冲区中的偏移量 */
  offset = y;           /* y可能是8位或16位，使用16位变量 */
  offset &= ~7;        /* 对齐到8的倍数（取整到字节边界） */
  offset *= u8g2_GetU8x8(u8g2)->display_info->tile_width;  /* 乘以tile宽度 */
  ptr = u8g2->tile_buf_ptr;  /* 获取缓冲区指针 */
  ptr += offset;        /* 移动到正确的行 */
  ptr += x;             /* 移动到正确的列 */
  
  if ( dir == 0 )    /* 水平线绘制 */
  {
      do
      {
#ifdef __unix
	assert(ptr < max_ptr);  /* Unix下的边界检查 */
#endif
	*ptr |= or_mask;     /* 设置像素位 */
	*ptr ^= xor_mask;    /* 根据颜色翻转 */
	ptr++;               /* 移动到下一个字节（水平方向） */
	len--;
      } while( len != 0 );
  }
  else               /* 垂直线绘制 */
  {
    do
    {
#ifdef __unix
      assert(ptr < max_ptr);  /* Unix下的边界检查 */
#endif
      *ptr |= or_mask;     /* 设置像素位 */
      *ptr ^= xor_mask;    /* 根据颜色翻转 */

      bit_pos++;           /* 位位置递增 */
      bit_pos &= 7;       /* 限制在0-7范围内 */

      len--;

      if ( bit_pos == 0 )  /* 超过当前字节，需要移动到下一个字节行 */
      {
	ptr+=u8g2->pixel_buf_width;  /* 移动到下一行（issue #148修复） */

	/* 重置掩码到最低位 */
	if ( u8g2->draw_color <= 1 )
	  or_mask  = 1;
	if ( u8g2->draw_color != 1 )
	  xor_mask = 1;
      }
      else
      {
	or_mask <<= 1;      /* 掩码左移，处理下一个位 */
	xor_mask <<= 1;
      }
    } while( len != 0 );
  }
}



#else /* U8G2_WITH_HVLINE_SPEED_OPTIMIZATION */

/**
 * @brief 绘制单个像素（垂直字节布局，LSB在顶部）
 *
 * 非优化版本，逐像素绘制。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     像素的x坐标（缓冲区内）
 * @param y     像素的y坐标（缓冲区内）
 */
static void u8g2_draw_pixel_vertical_top_lsb(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y)
{
  uint16_t offset;
  uint8_t *ptr;
  uint8_t bit_pos, mask;
  
  //assert(x >= u8g2->buf_x0);
  //assert(x < u8g2_GetU8x8(u8g2)->display_info->tile_width*8);
  //assert(y >= u8g2->buf_y0);
  //assert(y < u8g2_GetU8x8(u8g2)->display_info->tile_height*8);
  
  /* bytes are vertical, lsb on top (y=0), msb at bottom (y=7) */
  bit_pos = y;		/* overflow truncate is ok here... */
  bit_pos &= 7; 	/* ... because only the lowest 3 bits are needed */
  mask = 1;
  mask <<= bit_pos;

  offset = y;		/* y might be 8 or 16 bit, but we need 16 bit, so use a 16 bit variable */
  offset &= ~7;
  offset *= u8g2_GetU8x8(u8g2)->display_info->tile_width;
  ptr = u8g2->tile_buf_ptr;
  ptr += offset;
  ptr += x;


  if ( u8g2->draw_color <= 1 )
    *ptr |= mask;
  if ( u8g2->draw_color != 1 )
    *ptr ^= mask;

}

/**
 * @brief 垂直字节布局的线段绘制（非优化版本）
 *
 * 逐像素绘制，适用于代码大小敏感的场景。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标（本地缓冲区内）
 * @param y     线段起始点的y坐标（本地缓冲区内）
 * @param len   线段长度（像素），不能为0
 * @param dir   方向: 0=水平线（从左到右），1=垂直线（从上到下）
 *
 * @note 假设所有裁剪已完成
 */
void u8g2_ll_hvline_vertical_top_lsb(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  if ( dir == 0 )
  {
    do
    {
      u8g2_draw_pixel_vertical_top_lsb(u8g2, x, y);
      x++;
      len--;
    } while( len != 0 );
  }
  else
  {
    do
    {
      u8g2_draw_pixel_vertical_top_lsb(u8g2, x, y);
      y++;
      len--;
    } while( len != 0 );
  }
}


#endif /* U8G2_WITH_HVLINE_SPEED_OPTIMIZATION */

/*=================================================*/
/**
 * @brief 水平字节布局，LSB在右侧的线段绘制
 *
 * 适用于以下显示控制器：
 *   - SH1122, LD7032, ST7920, ST7986, LC7981
 *   - T6963, SED1330, RA8835, MAX7219, LS0
 *
 * 像素布局方式：
 *   每个字节表示水平方向的8个像素
 *   字节的bit7（MSB）对应左侧像素
 *   字节的bit0（LSB）对应右侧像素
 */

#ifdef U8G2_WITH_HVLINE_SPEED_OPTIMIZATION

/**
 * @brief 水平字节布局的线段绘制（速度优化版本）
 *
 * 优化版本直接操作内存，减少函数调用开销。
 * 适用于SH1122、ST7920等显示控制器。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标（本地缓冲区内）
 * @param y     线段起始点的y坐标（本地缓冲区内）
 * @param len   线段长度（像素），不能为0
 * @param dir   方向: 0=水平线（从左到右），1=垂直线（从上到下）
 *
 * @note 假设所有裁剪已完成
 */
/* SH1122, LD7032, ST7920, ST7986, LC7981, T6963, SED1330, RA8835, MAX7219, LS0 */
void u8g2_ll_hvline_horizontal_right_lsb(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  uint16_t offset;
  uint8_t *ptr;
  uint8_t bit_pos;
  uint8_t mask;
  uint8_t tile_width = u8g2_GetU8x8(u8g2)->display_info->tile_width;

  /* 计算位掩码：水平字节布局，MSB在左侧 */
  bit_pos = x;          /* 获取x坐标（溢出截断安全） */
  bit_pos &= 7;        /* 只需要低3位（0-7） */
  mask = 128;           /* 从最高位开始 */
  mask >>= bit_pos;     /* 右移到对应位置 */

  /* 计算像素在缓冲区中的偏移量 */
  offset = y;           /* y可能是8位或16位，使用16位变量 */
  offset *= tile_width; /* 乘以tile宽度 */
  offset += x>>3;       /* 加上x所在的字节偏移（x/8） */
  ptr = u8g2->tile_buf_ptr;  /* 获取缓冲区指针 */
  ptr += offset;        /* 移动到正确位置 */
  
  if ( dir == 0 )    /* 水平线绘制 */
  {

    do
    {

      if ( u8g2->draw_color <= 1 )
	*ptr |= mask;      /* 设置像素位 */
      if ( u8g2->draw_color != 1 )
	*ptr ^= mask;      /* 根据颜色翻转 */

      mask >>= 1;        /* 掩码右移，处理下一个像素 */
      if ( mask == 0 )   /* 超过当前字节，需要移动到下一个字节 */
      {
	mask = 128;      /* 重置掩码到最高位 */
        ptr++;           /* 移动到下一个字节 */
      }

      //x++;
      len--;
    } while( len != 0 );
  }
  else               /* 垂直线绘制 */
  {
    do
    {
      if ( u8g2->draw_color <= 1 )
	*ptr |= mask;      /* 设置像素位 */
      if ( u8g2->draw_color != 1 )
	*ptr ^= mask;      /* 根据颜色翻转 */

      ptr += tile_width; /* 移动到下一行（垂直方向） */
      //y++;
      len--;
    } while( len != 0 );
  }
}

#else /* U8G2_WITH_HVLINE_SPEED_OPTIMIZATION */


/**
 * @brief 绘制单个像素（水平字节布局，LSB在右侧）
 *
 * 非优化版本，逐像素绘制。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     像素的x坐标（缓冲区内）
 * @param y     像素的y坐标（缓冲区内）
 */
/* SH1122, LD7032, ST7920, ST7986, LC7981, T6963, SED1330, RA8835, MAX7219, LS0 */
static void u8g2_draw_pixel_horizontal_right_lsb(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y)
{
  uint16_t offset;
  uint8_t *ptr;
  uint8_t bit_pos, mask;

  //assert(x >= u8g2->buf_x0);
  //assert(x < u8g2_GetU8x8(u8g2)->display_info->tile_width*8);
  //assert(y >= u8g2->buf_y0);
  //assert(y < u8g2_GetU8x8(u8g2)->display_info->tile_height*8);
  
  /* bytes are vertical, lsb on top (y=0), msb at bottom (y=7) */
  bit_pos = x;		/* overflow truncate is ok here... */
  bit_pos &= 7; 	/* ... because only the lowest 3 bits are needed */
  mask = 128;
  mask >>= bit_pos;
  x >>= 3;

  offset = y;		/* y might be 8 or 16 bit, but we need 16 bit, so use a 16 bit variable */
  offset *= u8g2_GetU8x8(u8g2)->display_info->tile_width;
  offset += x;
  ptr = u8g2->tile_buf_ptr;
  ptr += offset;
  

  if ( u8g2->draw_color <= 1 )
    *ptr |= mask;
  if ( u8g2->draw_color != 1 )
    *ptr ^= mask;
  
}

/**
 * @brief 水平字节布局的线段绘制（非优化版本）
 *
 * 逐像素绘制，适用于代码大小敏感的场景。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标（本地缓冲区内）
 * @param y     线段起始点的y坐标（本地缓冲区内）
 * @param len   线段长度（像素），不能为0
 * @param dir   方向: 0=水平线（从左到右），1=垂直线（从上到下）
 *
 * @note 假设所有裁剪已完成
 */
/* SH1122, LD7032, ST7920, ST7986, LC7981, T6963, SED1330, RA8835, MAX7219, LS0 */
void u8g2_ll_hvline_horizontal_right_lsb(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  if ( dir == 0 )
  {
    do
    {
      u8g2_draw_pixel_horizontal_right_lsb(u8g2, x, y);
      x++;
      len--;
    } while( len != 0 );
  }
  else
  {
    do
    {
      u8g2_draw_pixel_horizontal_right_lsb(u8g2, x, y);
      y++;
      len--;
    } while( len != 0 );
  }
}

#endif /* U8G2_WITH_HVLINE_SPEED_OPTIMIZATION */
