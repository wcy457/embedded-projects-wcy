/**
 * @file u8x8_string.c
 * @brief u8x8字符串处理和多行文本绘制功能
 *
 * 本文件提供了字符串处理和多行文本绘制功能，包括：
 * - 统计字符串中的行数（以'\n'分隔）
 * - 获取指定行的起始位置
 * - 复制指定行到缓冲区
 * - 绘制居中对齐的UTF8文本行
 * - 绘制多行UTF8文本
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

/**
 * @brief 获取字符串中的行数
 *
 * 统计以'\n'分隔的行数。空字符串返回0。
 *
 * @param str 指向输入字符串的指针
 * @return 行数（至少为1，除非str为NULL）
 */
uint8_t u8x8_GetStringLineCnt(const char *str)
{
  char e;
  uint8_t line_cnt = 1;        /* 至少有1行 */
  if ( str == NULL )
    return 0;                  /* NULL字符串返回0 */
  for(;;)
  {
    e = *str;
    if ( e == '\0' )
      break;                   /* 字符串结束 */
    str++;
    if ( e == '\n' )
      line_cnt++;              /* 遇到换行符，行数+1 */
  }
  return line_cnt;
}


/**
 * @brief 获取指定行的起始位置
 *
 * 在以'\n'分隔的多行字符串中，返回指定行的起始指针。
 * 支持UTF8和普通字符串。
 *
 * 示例：str = "abc\nxyz"，line_idx = 1 返回指向"xyz"的指针。
 *
 * @param line_idx 行索引（从0开始）
 * @param str 指向输入字符串的指针
 * @return 指向指定行起始位置的指针，如果行不存在则返回NULL
 */
const char *u8x8_GetStringLineStart(uint8_t line_idx, const char *str )
{
  char e;
  uint8_t line_cnt = 1;

  if ( line_idx == 0 )
    return str;                 /* 第0行就是字符串开头 */

  for(;;)
  {
    e = *str;
    if ( e == '\0' )
      break;                    /* 字符串结束，未找到目标行 */
    str++;
    if ( e == '\n' )
    {
      if ( line_cnt == line_idx )
	return str;               /* 找到目标行，返回起始位置 */
      line_cnt++;
    }
  }
  return NULL;                  /* 行不存在 */
}

/**
 * @brief 复制指定行到目标缓冲区
 *
 * 从多行字符串中提取指定行并复制到目标缓冲区。
 * 复制在遇到'\n'或'\0'时停止。
 *
 * @param dest 目标缓冲区（注意：没有溢出检查，确保缓冲区足够大）
 * @param line_idx 行索引（从0开始）
 * @param str 指向输入字符串的指针
 */
void u8x8_CopyStringLine(char *dest, uint8_t line_idx, const char *str)
{
  if ( dest == NULL )
    return;
  str = u8x8_GetStringLineStart( line_idx, str );    /* 获取行起始位置 */
  if ( str != NULL )
  {
    for(;;)
    {
      if ( *str == '\n' || *str == '\0' )
	break;                    /* 遇到换行或字符串结束 */
      *dest = *str;             /* 复制字符 */
      dest++;
      str++;
    }
  }
  *dest = '\0';                 /* 添加字符串结束符 */
}

/**
 * @brief 绘制居中对齐的UTF8文本行
 *
 * 在指定宽度内绘制居中对齐的UTF8字符串。
 * 如果字符串宽度小于指定宽度，两侧用空格填充。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始X坐标（以tile为单位）
 * @param y 起始Y坐标（以tile为单位）
 * @param w 可用宽度（以tile为单位）
 * @param s 要绘制的UTF8字符串
 * @return 实际使用的宽度（以tile为单位）
 */
uint8_t u8x8_DrawUTF8Line(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t w, const char *s)
{
  uint8_t d, lw;
  uint8_t cx, dx;

  d = 0;

  lw = u8x8_GetUTF8Len(u8x8, s);    /* 获取字符串的显示宽度 */
  if ( lw < w )
  {
    d = w;
    d -= lw;                          /* 计算剩余空间 */
    d /= 2;                          /* 计算左侧填充量 */
  }

  cx = x;
  dx = cx + d;
  /* 左侧填充空格 */
  while( cx < dx )
  {
    u8x8_DrawUTF8(u8x8, cx, y, " ");
    cx++;
  }
  cx += u8x8_DrawUTF8(u8x8, cx, y, s);    /* 绘制字符串 */
  dx = x + w;
  /* 右侧填充空格 */
  while( cx < dx )
  {
    u8x8_DrawUTF8(u8x8, cx, y, " ");
    cx++;
  }
  cx -= x;                          /* 计算实际使用的宽度 */
  return cx;
}

/**
 * @brief 绘制多行UTF8文本
 *
 * 在指定位置绘制多行居中对齐的UTF8文本。
 * 行之间用'\n'分隔。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始X坐标（以tile为单位）
 * @param y 起始Y坐标（以tile为单位）
 * @param w 每行的可用宽度（以tile为单位）
 * @param s 要绘制的多行UTF8字符串（以'\n'分隔）
 * @return 总行数（如果s为NULL则返回0）
 */
uint8_t u8x8_DrawUTF8Lines(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t w, const char *s)
{
  uint8_t i;
  uint8_t cnt;
  cnt = u8x8_GetStringLineCnt(s);    /* 获取总行数 */
  for( i = 0; i < cnt; i++ )
  {
    /* 逐行绘制，每行自动换到下一行 */
    u8x8_DrawUTF8Line(u8x8, x, y, w, u8x8_GetStringLineStart(i, s));
    y++;
  }
  return cnt;
}
