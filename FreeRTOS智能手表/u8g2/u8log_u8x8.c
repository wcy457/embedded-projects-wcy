/*

  u8log_u8x8.c
  

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
  文件: u8log_u8x8.c
  功能: u8log日志模块与u8x8显示驱动的回调接口
        提供将u8log缓冲区中的日志文本绘制到u8x8字符显示设备上的功能。
        u8x8是u8g2库的底层字符显示模块，适用于不支持像素级绘制的字符型LCD。

  主要函数:
    - u8x8_DrawLogLine(): 绘制日志缓冲区中的一行文本
    - u8x8_DrawLog(): 绘制整个日志缓冲区的内容
    - u8log_u8x8_cb(): u8log的回调函数，用于u8x8显示设备自动刷新日志
*/

#include "u8x8.h"

/*
  函数: u8x8_DrawLogLine
  功能: 将日志缓冲区中指定行的内容绘制到u8x8显示设备上
  参数:
    u8x8   - u8x8显示设备指针
    disp_x - 显示设备上的起始X坐标（字符列位置）
    disp_y - 显示设备上的起始Y坐标（字符行位置）
    buf_y  - 日志缓冲区中要绘制的行号
    u8log  - 日志结构体指针，包含屏幕缓冲区和尺寸信息
  返回值: 无
*/
static void u8x8_DrawLogLine(u8x8_t *u8x8, uint8_t disp_x, uint8_t disp_y, uint8_t buf_y, u8log_t *u8log) U8X8_NOINLINE;
static void u8x8_DrawLogLine(u8x8_t *u8x8, uint8_t disp_x, uint8_t disp_y, uint8_t buf_y, u8log_t *u8log)
{
  uint8_t buf_x;
  uint8_t c;
  for( buf_x = 0; buf_x < u8log->width; buf_x++ )
  {
    c = u8log->screen_buffer[buf_y * u8log->width + buf_x];  // 从缓冲区读取当前字符
    u8x8_DrawGlyph(u8x8, disp_x, disp_y, c);                 // 在显示设备上绘制该字符
    disp_x++;                                                   // 移动到下一个字符位置
  }
}

/*
  函数: u8x8_DrawLog
  功能: 将整个日志缓冲区的内容绘制到u8x8显示设备上
        逐行遍历日志缓冲区，调用u8x8_DrawLogLine绘制每一行
  参数:
    u8x8  - u8x8显示设备指针
    x     - 显示起始X坐标（字符列位置）
    y     - 显示起始Y坐标（字符行位置）
    u8log - 日志结构体指针
  返回值: 无
*/
void u8x8_DrawLog(u8x8_t *u8x8, uint8_t x, uint8_t y, u8log_t *u8log)
{
  uint8_t buf_y;
  for( buf_y = 0; buf_y < u8log->height; buf_y++ )   // 遍历每一行
  {
    u8x8_DrawLogLine(u8x8, x, y, buf_y, u8log);      // 绘制当前行
    y++;                                                 // 移动到下一行显示位置
  }
}


/*
  函数: u8log_u8x8_cb
  功能: u8log日志模块的u8x8回调函数
        当日志内容发生变化时自动被调用，用于刷新u8x8显示设备上的日志显示。
        支持全屏刷新和单行刷新两种模式：
        - is_redraw_all: 刷新整个屏幕
        - is_redraw_line: 仅刷新标记的行（性能优化）
  参数:
    u8log - 日志结构体指针，其aux_data成员应指向u8x8_t显示设备
  返回值: 无
*/
void u8log_u8x8_cb(u8log_t * u8log)
{
  u8x8_t *u8x8 = (u8x8_t *)(u8log->aux_data);   // 从aux_data获取u8x8显示设备指针
  if ( u8log->is_redraw_all )                       // 如果需要全屏刷新
  {
    u8x8_DrawLog(u8x8, 0, 0, u8log);              // 绘制整个日志缓冲区
  }
  else if ( u8log->is_redraw_line )                 // 如果只需要刷新单行
  {
    u8x8_DrawLogLine(u8x8, 0, u8log->redraw_line, u8log->redraw_line, u8log);  // 仅绘制需要刷新的行
  }
}

