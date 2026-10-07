/**
 * @file u8x8_capture.c
 * @brief u8x8屏幕截图功能实现
 *
 * 本文件提供了u8x8显示内容的截图和导出功能，支持：
 * - 从显示缓冲区读取单个像素（支持多种内存架构）
 * - 导出为PBM（Portable Bitmap）格式
 * - 导出为XBM（X BitMap）格式
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

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

#include "u8x8.h"

/*========================================================*/

/**
 * @brief 从显示缓冲区获取像素值（垂直LSB内存架构）
 *
 * 内存布局：垂直排列，每8个像素组成一个字节，LSB在上。
 * 适用于大多数常见的OLED控制器（如SSD1306）。
 *
 * @param x 像素X坐标
 * @param y 像素Y坐标
 * @param dest_ptr 指向显示缓冲区的指针
 * @param tile_width 缓冲区的tile宽度（以8像素为单位）
 * @return 像素值（0或1）
 */
uint8_t u8x8_capture_get_pixel_1(uint16_t x, uint16_t y, uint8_t *dest_ptr, uint8_t tile_width)
{
  dest_ptr += (y/8)*tile_width*8;  /* 计算行偏移（每行tile_width*8字节） */
  y &= 7;                          /* 取y在字节内的位偏移（0-7） */
  dest_ptr += x;                   /* 计算列偏移 */
  if ( (*dest_ptr & (1<<y)) == 0 ) /* 检查对应位 */
    return 0;
  return 1;
}

/**
 * @brief 从显示缓冲区获取像素值（水平LSB内存架构）
 *
 * 内存布局：水平排列，每8个像素组成一个字节，MSB在左。
 * 适用于SH1122、LD7032、ST7920、T6963等控制器。
 *
 * @param x 像素X坐标
 * @param y 像素Y坐标
 * @param dest_ptr 指向显示缓冲区的指针
 * @param tile_width 缓冲区的tile宽度（以8像素为单位）
 * @return 像素值（0或1）
 */
uint8_t u8x8_capture_get_pixel_2(uint16_t x, uint16_t y, uint8_t *dest_ptr, uint8_t tile_width)
{
  y *= tile_width;                      /* 计算行在缓冲区中的起始位置 */
  dest_ptr += y;
  dest_ptr += x>>3;                     /* 计算字节偏移（x/8） */
  if ( (*dest_ptr & (128>>(x&7))) == 0 ) /* 检查对应位（MSB在左） */
    return 0;
  return 1;
}

/**
 * @brief 写入PBM文件头
 *
 * PBM (Portable Bitmap) 是一种简单的黑白图像格式。
 * 文件头包含格式标识"P1"和图像宽高。
 *
 * @param tile_width 图像宽度（以tile为单位，1tile=8像素）
 * @param tile_height 图像高度（以tile为单位）
 * @param out 输出回调函数，用于输出字符串
 */
void u8x8_capture_write_pbm_pre(uint8_t tile_width, uint8_t tile_height, void (*out)(const char *s))
{
  out("P1\n");                                           /* PBM格式标识 */
  out(u8x8_utoa((uint16_t)tile_width*8));                /* 图像宽度（像素） */
  out("\n");
  out(u8x8_utoa((uint16_t)tile_height*8));               /* 图像高度（像素） */
  out("\n");
}

/**
 * @brief 将显示缓冲区导出为PBM格式数据
 *
 * 逐行逐像素输出，每个像素用'0'或'1'表示。
 *
 * @param buffer 指向显示缓冲区的指针
 * @param tile_width 缓冲区宽度（以tile为单位）
 * @param tile_height 缓冲区高度（以tile为单位）
 * @param get_pixel 像素读取回调函数
 * @param out 输出回调函数
 */
void u8x8_capture_write_pbm_buffer(uint8_t *buffer, uint8_t tile_width, uint8_t tile_height, uint8_t (*get_pixel)(uint16_t x, uint16_t y, uint8_t *dest_ptr, uint8_t tile_width), void (*out)(const char *s))
{
  uint16_t x, y;
  uint16_t w, h;

  w = tile_width;
  w *= 8;        /* 转换为像素宽度 */
  h = tile_height;
  h *= 8;        /* 转换为像素高度 */

  for( y = 0; y < h; y++)           /* 遍历每一行 */
  {
    for( x = 0; x < w; x++)         /* 遍历每一列 */
    {
      if ( get_pixel(x, y, buffer, tile_width) )
	out("1");                     /* 亮像素 */
      else
	out("0");                     /* 暗像素 */
    }
    out("\n");                        /* 行结束换行 */
  }
}




/**
 * @brief 写入XBM文件头
 *
 * XBM (X BitMap) 是一种C语言风格的位图格式，可直接嵌入源代码。
 * 文件头包含宽度和高度的宏定义。
 *
 * @param tile_width 图像宽度（以tile为单位）
 * @param tile_height 图像高度（以tile为单位）
 * @param out 输出回调函数
 */
void u8x8_capture_write_xbm_pre(uint8_t tile_width, uint8_t tile_height, void (*out)(const char *s))
{
  out("#define xbm_width ");
  out(u8x8_utoa((uint16_t)tile_width*8));     /* 输出宽度 */
  out("\n");
  out("#define xbm_height ");
  out(u8x8_utoa((uint16_t)tile_height*8));    /* 输出高度 */
  out("\n");
  out("static unsigned char xbm_bits[] = {\n");  /* 数据数组开始 */
}

/**
 * @brief 将显示缓冲区导出为XBM格式数据
 *
 * XBM格式特点：每行从右到左编码，MSB在左。
 * 每8个像素打包成一个字节，以十六进制输出。
 *
 * @param buffer 指向显示缓冲区的指针
 * @param tile_width 缓冲区宽度（以tile为单位）
 * @param tile_height 缓冲区高度（以tile为单位）
 * @param get_pixel 像素读取回调函数
 * @param out 输出回调函数
 */
void u8x8_capture_write_xbm_buffer(uint8_t *buffer, uint8_t tile_width, uint8_t tile_height, uint8_t (*get_pixel)(uint16_t x, uint16_t y, uint8_t *dest_ptr, uint8_t tile_width), void (*out)(const char *s))
{
  uint16_t x, y;
  uint16_t w, h;
  uint8_t v, b;
  char s[2];
  s[1] = '\0';

  w = tile_width;
  w *= 8;        /* 转换为像素宽度 */
  h = tile_height;
  h *= 8;        /* 转换为像素高度 */

  y = 0;
  for(;;)
  {
    x = 0;
    for(;;)
    {
      v = 0;
      /* 将8个像素打包成一个字节（XBM格式：从右到左） */
      for( b = 0; b < 8; b++ )
      {
	v <<= 1;
	if ( get_pixel(x+7-b, y, buffer, tile_width) )
	  v |= 1;       /* 设置最低位 */
      }
      /* 输出十六进制值 */
      out("0x");
      s[0] = (v>>4);           /* 高4位 */
      if ( s[0] <= 9 )
	s[0] += '0';           /* 0-9 */
      else
	s[0] += 'a'-10;        /* a-f */
      out(s);
      s[0] = (v&15);           /* 低4位 */
      if ( s[0] <= 9 )
	s[0] += '0';
      else
	s[0] += 'a'-10;
      out(s);
      x += 8;
      if ( x >= w )
	break;
      out(",");                /* 字节间分隔符 */
    }
    y++;
    if ( y >= h )
      break;
    out(",");
    out("\n");                 /* 行结束 */
  }
  out("};\n");                 /* 数据数组结束 */
}



/*========================================================*/

#ifdef NOT_YET_IMPLEMENTED_U8X8_SCREEN_CAPTURE

struct _u8x8_capture_struct
{
  u8x8_msg_cb old_cb;
  uint8_t *buffer;	/* tile_width*tile_height*8 bytes */
  uint8_t tile_width;
  uint8_t tile_height;
};
typedef struct _u8x8_capture_struct u8x8_capture_t;


u8x8_capture_t u8x8_capture;


static void u8x8_capture_memory_copy(uint8_t *dest, uint8_t *src, uint16_t cnt)
{
  while( cnt > 0 )
  {
    *dest++ = *src++;
    cnt--;
  }
}

static void u8x8_capture_DrawTiles(u8x8_capture_t *capture, uint8_t tx, uint8_t ty, uint8_t tile_cnt, uint8_t *tile_ptr)
{
  uint8_t *dest_ptr = capture->buffer;
  //printf("tile pos: %d %d, cnt=%d\n", tx, ty, tile_cnt);
  if ( dest_ptr == NULL )
    return;
  dest_ptr += (uint16_t)ty*capture->tile_width*8;
  dest_ptr += (uint16_t)tx*8;
  u8x8_capture_memory_copy(dest_ptr, tile_ptr, tile_cnt*8);
}

uint8_t u8x8_d_capture(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if (  msg ==  U8X8_MSG_DISPLAY_DRAW_TILE )
  {
    uint8_t x, y, c;
    uint8_t *ptr;
    x = ((u8x8_tile_t *)arg_ptr)->x_pos;    
    y = ((u8x8_tile_t *)arg_ptr)->y_pos;
    c = ((u8x8_tile_t *)arg_ptr)->cnt;
    ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;
    do
    {
      u8x8_capture_DrawTiles(&u8x8_capture, x, y, c, ptr);
      x += c;
      arg_int--;
    } while( arg_int > 0 );
  }
  return u8x8_capture.old_cb(u8x8, msg, arg_int, arg_ptr);
}

uint8_t u8x8_GetCaptureMemoryPixel(u8x8_t *u8x8, uint16_t x, uint16_t y)
{
  return u8x8_capture_GetPixel(&u8x8_capture, x, y);
}

/* memory: tile_width*tile_height*8 bytes */
void u8x8_ConnectCapture(u8x8_t *u8x8, uint8_t tile_width, uint8_t tile_height, uint8_t *memory)
{
  if ( u8x8->display_cb == u8x8_d_capture )
    return;	/* do nothing, capture already installed */

  u8x8_capture.buffer = memory;	/* tile_width*tile_height*8 bytes */
  u8x8_capture.tile_width = tile_width;
  u8x8_capture.tile_height = tile_height;
  u8x8_capture.old_cb = u8x8->display_cb;
  u8x8->display_cb = u8x8_d_capture;
  return;
}

#endif
