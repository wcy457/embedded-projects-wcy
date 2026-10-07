/*

  u8log_u8g2.c
  

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
  文件: u8log_u8g2.c
  功能: u8log日志模块与u8g2图形库的回调接口
        提供将u8log缓冲区中的日志文本通过u8g2图形库绘制到像素级显示设备上的功能。
        u8g2支持矢量字体和像素级绘制，适用于OLED、LCD等图形显示屏。

  主要函数:
    - u8g2_DrawLog(): 在指定位置绘制整个日志缓冲区的文本内容
    - u8log_u8g2_cb(): u8log的回调函数，配合u8g2的分页机制自动刷新日志显示
*/

#include "u8g2.h"
/*
  Draw the u8log text at the specified x/y position.
  x/y position is the reference position of the first char of the first line.
  the line height is 
    u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) + line_height_offset;
  line_height_offset can be set with u8log_SetLineHeightOffset()
  Use
    u8g2_SetFontRefHeightText(u8g2_t *u8g2);
    u8g2_SetFontRefHeightExtendedText(u8g2_t *u8g2);
    u8g2_SetFontRefHeightAll(u8g2_t *u8g2);
  to change the return values for u8g2_GetAscent and u8g2_GetDescent

*/
/*
  函数: u8g2_DrawLog
  功能: 使用u8g2图形库在指定位置绘制日志缓冲区的全部内容
        逐行逐字符绘制，行高由字体的ascent/descent和line_height_offset决定
  参数:
    u8g2  - u8g2图形设备指针
    x     - 绘制起始X坐标（像素位置）
    y     - 绘制起始Y坐标（像素位置，为第一行文字的基线位置）
    u8log - 日志结构体指针，包含屏幕缓冲区和尺寸信息
  返回值: 无
*/
void u8g2_DrawLog(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8log_t *u8log)
{
  u8g2_uint_t disp_x, disp_y;
  uint8_t buf_x, buf_y;
  uint8_t c;

  disp_y = y;
  u8g2_SetFontDirection(u8g2, 0);                            // 设置字体方向为默认（从左到右）
  for( buf_y = 0; buf_y < u8log->height; buf_y++ )            // 遍历日志缓冲区的每一行
  {
    disp_x = x;
    for( buf_x = 0; buf_x < u8log->width; buf_x++ )          // 遍历行中的每个字符
    {
      c = u8log->screen_buffer[buf_y * u8log->width + buf_x]; // 从缓冲区读取字符
      disp_x += u8g2_DrawGlyph(u8g2, disp_x, disp_y, c);     // 绘制字符，返回值为字符宽度
    }
    disp_y += u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2);  // 计算行高（字体高度）
    disp_y += u8log->line_height_offset;                       // 加上用户自定义的行高偏移
  }
}

/*
  u8lib callback for u8g2
  
  Only font direction 0 is supported: u8g2_SetFontDirection(u8g2, 0)
  Use
    u8g2_SetFontRefHeightText(u8g2_t *u8g2);
    u8g2_SetFontRefHeightExtendedText(u8g2_t *u8g2);
    u8g2_SetFontRefHeightAll(u8g2_t *u8g2);
  to change the top offset and the line height and
    u8log_SetLineHeightOffset(u8log_t *u8log, int8_t line_height_offset)
  to change the line height.
  
*/
/*
  函数: u8log_u8g2_cb
  功能: u8log日志模块的u8g2回调函数
        当日志内容发生变化时自动被调用，使用u8g2的分页渲染机制刷新显示。
        u8g2的分页机制（FirstPage/NextPage）适用于RAM有限的嵌入式系统，
        每次只渲染显示的一部分，减少内存占用。
  参数:
    u8log - 日志结构体指针，其aux_data成员应指向u8g2_t图形设备
  返回值: 无
*/
void u8log_u8g2_cb(u8log_t * u8log)
{
  u8g2_t *u8g2 = (u8g2_t *)(u8log->aux_data);   // 从aux_data获取u8g2图形设备指针
  if ( u8log->is_redraw_line || u8log->is_redraw_all )  // 检查是否需要刷新
  {
    u8g2_FirstPage(u8g2);                          // 开始分页渲染
    do
    {
      u8g2_DrawLog( u8g2, 0, u8g2_GetAscent(u8g2), u8log);  // 绘制日志内容
    }
    while( u8g2_NextPage(u8g2) );                  // 继续下一页直到所有页渲染完成
  }
}

