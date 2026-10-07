/*

  u8log.c
  

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2018, olikraus@gmail.com
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

/*
  文件: u8log.c
  功能: u8log日志模块的核心实现
        提供一个轻量级的、基于字符缓冲区的日志/终端系统。
        适用于嵌入式系统中在显示屏上显示调试信息、运行日志等文本内容。

  主要特性:
    - 维护一个字符屏幕缓冲区（screen_buffer），模拟终端显示
    - 支持光标定位、自动换行、滚动等终端功能
    - 支持控制字符：\n（换行）、\r（回车）、\t（制表）、\f（清屏）
    - 通过回调函数机制通知显示驱动刷新屏幕
    - 支持单行刷新和全屏刷新两种模式，优化显示性能

  典型使用流程:
    1. 调用 u8log_Init() 初始化日志结构体
    2. 调用 u8log_SetCallback() 设置刷新回调函数
    3. 调用 u8log_WriteString() 或 u8log_WriteChar() 写入日志内容
    4. 回调函数自动被触发，将日志内容刷新到显示设备上
*/

#include <stdint.h>
#include <string.h>
#include "u8x8.h"


/*
static uint8_t u8log_is_on_screen(u8log_t *u8log, uint8_t x, uint8_t y)
{
  if ( x >= u8log->width )
    return 0;
  if ( y >= u8log->height )
    return 0;
  return 1;
}
*/

/*
  函数: u8log_clear_screen (内部函数)
  功能: 清空日志屏幕缓冲区，将所有位置填充为空格字符
  参数:
    u8log - 日志结构体指针
  返回值: 无
*/
static void u8log_clear_screen(u8log_t *u8log)
{
  uint8_t *dest = u8log->screen_buffer;  // 指向缓冲区起始位置
  uint16_t cnt = u8log->height;
  cnt *= u8log->width;                   // 计算缓冲区总字符数（宽×高）
  do
  {
    *dest++ = ' ';                       // 用空格填充每个位置
    cnt--;
  } while( cnt > 0 );

}


/*
  函数: u8log_scroll_up (内部函数)
  功能: 将屏幕缓冲区内容向上滚动一行
        将第2行到最后一行的内容复制到第1行到倒数第二行，
        然后将最后一行清空为空格。
        根据刷新模式设置相应的重绘标志。
  参数:
    u8log - 日志结构体指针
  返回值: 无
*/
static void u8log_scroll_up(u8log_t *u8log)
{
  uint8_t *dest = u8log->screen_buffer;           // 目标：缓冲区起始（第1行）
  uint8_t *src = dest+u8log->width;               // 源：第2行起始位置
  uint16_t cnt = u8log->height;
  cnt--;
  cnt *= u8log->width;                            // 需要复制的字符数（总行数-1）×宽度
  do
  {
    *dest++ = *src++;                              // 将上一行内容复制到当前行
    cnt--;
  } while( cnt > 0 );
  cnt = u8log->width;
  do
  {
    *dest++ = ' ';                                 // 最后一行清空为空格
    cnt--;
  } while(cnt > 0);

  if ( u8log->is_redraw_line_for_each_char )       // 如果是逐字符刷新模式
    u8log->is_redraw_all = 1;                      // 标记需要全屏刷新
  else
    u8log->is_redraw_all_required_for_next_nl = 1; // 延迟到下一个换行时再全屏刷新
}

/*
  函数: u8log_cursor_on_screen (内部函数)
  功能: 确保光标在屏幕可见范围内
        如果光标X坐标超出宽度，则换行到下一行起始位置。
        如果光标Y坐标超出屏幕高度，则向上滚动屏幕直到光标可见。
  参数:
    u8log - 日志结构体指针
  返回值: 无
*/
static void u8log_cursor_on_screen(u8log_t *u8log)
{
  if ( u8log->cursor_x >= u8log->width )     // 光标X超出屏幕宽度
  {
    u8log->cursor_x = 0;                      // 回到行首
    u8log->cursor_y++;                         // 移到下一行
  }
  while ( u8log->cursor_y >= u8log->height )  // 光标Y超出屏幕高度
  {
    u8log_scroll_up(u8log);                    // 向上滚动一行
    u8log->cursor_y--;                         // 光标上移一行
  }
}

/*
  函数: u8log_write_to_screen (内部函数)
  功能: 将一个可打印字符写入屏幕缓冲区的当前光标位置
        写入后光标自动右移一位。如果启用了逐字符刷新模式，
        则标记当前行需要重绘。
  参数:
    u8log - 日志结构体指针
    c     - 要写入的字符
  返回值: 无
*/
static void u8log_write_to_screen(u8log_t *u8log, uint8_t c)
{
  u8log_cursor_on_screen(u8log);                                         // 确保光标在屏幕内
  u8log->screen_buffer[u8log->cursor_y * u8log->width + u8log->cursor_x] = c;  // 写入字符
  u8log->cursor_x++;                                                      // 光标右移

  if ( u8log->is_redraw_line_for_each_char )                              // 逐字符刷新模式
  {
    u8log->is_redraw_line = 1;                                            // 标记需要重绘行
    u8log->redraw_line = u8log->cursor_y;                                 // 记录需要重绘的行号
  }
}

/*
  函数: u8log_write_char (内部函数)
  功能: 处理单个字符的写入，支持控制字符和可打印字符
        控制字符处理:
          \n (10) - 换行：光标移到下一行行首，标记当前行需要重绘
          \r (13) - 回车：光标移到当前行行首，标记当前行需要重绘
          \t (9)  - 制表符：光标跳到下一个8字符对齐的制表位
          \f (12) - 换页：清空整个屏幕，重置光标到左上角
        其他字符直接写入屏幕缓冲区
  参数:
    u8log - 日志结构体指针
    c     - 要处理的字符（可以是控制字符或可打印字符）
  返回值: 无
*/
void u8log_write_char(u8log_t *u8log, uint8_t c)
{
  switch(c)
  {
    case '\n':	// 换行符 (ASCII 10)
      u8log->is_redraw_line = 1;                           // 标记当前行需要重绘
      u8log->redraw_line = u8log->cursor_y;                // 记录需要重绘的行号
      if ( u8log->is_redraw_all_required_for_next_nl )     // 如果之前滚动过，需要全屏刷新
	u8log->is_redraw_all = 1;
      u8log->is_redraw_all_required_for_next_nl = 0;      // 清除延迟全屏刷新标志
      u8log->cursor_y++;                                   // 光标下移一行
      u8log->cursor_x = 0;                                 // 光标回到行首
      break;
    case '\r':	// 回车符 (ASCII 13)
      u8log->is_redraw_line = 1;                           // 标记当前行需要重绘
      u8log->redraw_line = u8log->cursor_y;                // 记录需要重绘的行号
      u8log->cursor_x = 0;                                 // 光标回到行首
      break;
    case '\t':	// 制表符 (ASCII 9)
      u8log->cursor_x = (u8log->cursor_x + 8) & 0xf8;    // 跳到下一个8字符对齐的制表位
      break;
    case '\f':	// 换页符 (ASCII 12)
      u8log_clear_screen(u8log);                           // 清空屏幕
      u8log->is_redraw_all = 1;                            // 标记全屏需要重绘
      u8log->cursor_x = 0;                                 // 光标回到左上角
      u8log->cursor_y = 0;
      break;
    default:                                               // 可打印字符
      u8log_write_to_screen(u8log, c);                     // 写入屏幕缓冲区
      break;
  }
}

/*
  函数: u8log_Init
  功能: 初始化日志结构体
        清零所有成员变量，设置屏幕缓冲区的宽度和高度，
        并将缓冲区指针关联到用户提供的内存区域。
        初始化后屏幕被清空为空格。
  参数:
    u8log  - 日志结构体指针（由调用者分配内存）
    width  - 屏幕缓冲区的宽度（字符列数）
    height - 屏幕缓冲区的高度（字符行数）
    buf    - 用户提供的屏幕缓冲区指针，大小至少为 width*height 字节
  返回值: 无
*/
void u8log_Init(u8log_t *u8log, uint8_t width, uint8_t height, uint8_t *buf)
{
  memset(u8log, 0, sizeof(u8log_t));    // 将整个结构体清零
  u8log->width = width;                  // 设置屏幕宽度
  u8log->height = height;                // 设置屏幕高度
  u8log->screen_buffer = buf;            // 关联用户提供的缓冲区
  u8log_clear_screen(u8log);             // 清空屏幕为空格
}

/*
  函数: u8log_SetCallback
  功能: 设置日志模块的回调函数和辅助数据
        当日志内容发生变化需要刷新显示时，回调函数会被调用。
        aux_data通常用于传递显示设备的指针（如u8g2_t*或u8x8_t*）。
  参数:
    u8log    - 日志结构体指针
    cb       - 回调函数指针，当需要刷新显示时被调用
    aux_data - 辅助数据指针，回调函数中可通过u8log->aux_data访问
  返回值: 无
*/
void u8log_SetCallback(u8log_t *u8log, u8log_cb cb, void *aux_data)
{
  u8log->cb = cb;                  // 设置回调函数
  u8log->aux_data = aux_data;      // 设置辅助数据（通常是显示设备指针）
}

/*
  函数: u8log_SetRedrawMode
  功能: 设置日志的重绘模式
        is_redraw_line_for_each_char = 1: 每写入一个字符就标记当前行需要重绘
          适用于需要实时显示的场景，但会增加刷新频率
        is_redraw_line_for_each_char = 0: 仅在换行或滚动时标记重绘
          减少刷新频率，适用于不需要逐字符实时显示的场景
  参数:
    u8log                       - 日志结构体指针
    is_redraw_line_for_each_char - 重绘模式标志（1=逐字符重绘，0=仅换行时重绘）
  返回值: 无
*/
void u8log_SetRedrawMode(u8log_t *u8log, uint8_t is_redraw_line_for_each_char)
{
  u8log->is_redraw_line_for_each_char = is_redraw_line_for_each_char;  // 设置重绘模式
}

/*
  函数: u8log_SetLineHeightOffset
  功能: 设置行高偏移量，用于调整u8g2绘制日志时的行间距
        偏移量可以为正数（增加行间距）或负数（减少行间距），默认为0
  参数:
    u8log              - 日志结构体指针
    line_height_offset - 行高偏移值（有符号整数，单位为像素）
  返回值: 无
*/
void u8log_SetLineHeightOffset(u8log_t *u8log, int8_t line_height_offset)
{
  u8log->line_height_offset = line_height_offset;  // 设置行高偏移
}


/*
  函数: u8log_WriteChar
  功能: 向日志写入单个字符（用户API）
        处理字符后，检查是否需要刷新显示，如果需要则调用回调函数。
        与内部函数u8log_write_char的区别是：此函数会触发显示刷新。
  参数:
    u8log - 日志结构体指针
    c     - 要写入的字符
  返回值: 无
*/
void u8log_WriteChar(u8log_t *u8log, uint8_t c)
{
  u8log_write_char(u8log, c);                          // 处理字符（写入缓冲区或处理控制字符）
  if ( u8log->is_redraw_line || u8log->is_redraw_all ) // 检查是否需要刷新显示
  {
    if ( u8log->cb != 0 )                              // 如果设置了回调函数
    {
      u8log->cb(u8log);                                // 调用回调函数刷新显示
    }
    u8log->is_redraw_line = 0;                         // 清除单行刷新标志
    u8log->is_redraw_all = 0;                          // 清除全屏刷新标志
  }
}

/*
  函数: u8log_WriteString
  功能: 向日志写入一个以null结尾的字符串（用户API）
        逐字符调用u8log_WriteChar处理，支持字符串中的控制字符。
  参数:
    u8log - 日志结构体指针
    s     - 要写入的字符串指针（以'\0'结尾）
  返回值: 无
*/
void u8log_WriteString(u8log_t *u8log, const char *s)
{
  while( *s != '\0' )                    // 遍历字符串直到结束符
  {
    u8log_WriteChar(u8log, *s);          // 逐字符写入
    s++;
  }
}

/*
  函数: u8log_WriteHexHalfByte (内部函数)
  功能: 将一个4位半字节（nibble）以十六进制字符形式写入日志
  参数:
    u8log - 日志结构体指针
    b     - 要转换的数值（仅低4位有效）
  返回值: 无
*/
static void u8log_WriteHexHalfByte(u8log_t *u8log, uint8_t b) U8X8_NOINLINE;
static void u8log_WriteHexHalfByte(u8log_t *u8log, uint8_t b)
{
  b &= 0x0f;                                    // 只保留低4位
  if ( b < 10 )
    u8log_WriteChar(u8log, b+'0');               // 0-9: 写入'0'-'9'
  else
    u8log_WriteChar(u8log, b+'a'-10);            // 10-15: 写入'a'-'f'
}

/*
  函数: u8log_WriteHex8
  功能: 将一个8位无符号整数以两位十六进制形式写入日志
        例如：值255将显示为"ff"
  参数:
    u8log - 日志结构体指针
    b     - 要转换的8位无符号整数
  返回值: 无
*/
void u8log_WriteHex8(u8log_t *u8log, uint8_t b)
{
  u8log_WriteHexHalfByte(u8log, b >> 4);   // 写入高4位
  u8log_WriteHexHalfByte(u8log, b);        // 写入低4位
}

/*
  函数: u8log_WriteHex16
  功能: 将一个16位无符号整数以四位十六进制形式写入日志
        例如：值0x1234将显示为"1234"
  参数:
    u8log - 日志结构体指针
    v     - 要转换的16位无符号整数
  返回值: 无
*/
void u8log_WriteHex16(u8log_t *u8log, uint16_t v)
{
  u8log_WriteHex8(u8log, v>>8);   // 写入高8位（高字节）
  u8log_WriteHex8(u8log, v);      // 写入低8位（低字节）
}

/*
  函数: u8log_WriteHex32
  功能: 将一个32位无符号整数以八位十六进制形式写入日志
        例如：值0x12345678将显示为"12345678"
  参数:
    u8log - 日志结构体指针
    v     - 要转换的32位无符号整数
  返回值: 无
*/
void u8log_WriteHex32(u8log_t *u8log, uint32_t v)
{
  u8log_WriteHex16(u8log, v>>16);   // 写入高16位
  u8log_WriteHex16(u8log, v);       // 写入低16位
}

/*
  函数: u8log_WriteDec8
  功能: 将一个8位无符号整数以十进制形式写入日志
        使用u8x8_u8toa进行转换，支持指定位数（不足位数时前面补空格）
  参数:
    u8log - 日志结构体指针
    v     - 要转换的8位无符号整数（0-255）
    d     - 显示的位数（1-3），例如d=3时值5显示为"  5"
  返回值: 无
*/
void u8log_WriteDec8(u8log_t *u8log, uint8_t v, uint8_t d)
{
  u8log_WriteString(u8log, u8x8_u8toa(v, d));  // 转换为十进制字符串后写入
}

/*
  函数: u8log_WriteDec16
  功能: 将一个16位无符号整数以十进制形式写入日志
        使用u8x8_u16toa进行转换，支持指定位数（不足位数时前面补空格）
  参数:
    u8log - 日志结构体指针
    v     - 要转换的16位无符号整数（0-65535）
    d     - 显示的位数（1-5），例如d=5时值42显示为 "   42"
  返回值: 无
*/
void u8log_WriteDec16(u8log_t *u8log, uint16_t v, uint8_t d)
{
  u8log_WriteString(u8log, u8x8_u16toa(v, d));  // 转换为十进制字符串后写入
}
