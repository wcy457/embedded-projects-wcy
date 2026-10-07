/**
 * @file u8x8_u16toa.c
 * @brief 16位无符号整数转字符串功能
 *
 * 本文件提供了将16位无符号整数（0-65535）转换为字符串的功能。
 * 包含：
 * - u8x8_u16toap: 转换为5位字符串（带前导零）
 * - u8x8_u16toa: 转换为指定位数的字符串
 * - u8x8_utoa: 转换为无前导零的字符串
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
 * @brief 将16位无符号整数转换为5位字符串（带前导零）
 *
 * 将0-65535的值转换为5位ASCII字符串（如"00000"、"00123"、"65535"）。
 * 结果存入dest缓冲区（需要至少6字节）。
 *
 * @param dest 目标缓冲区（至少6字节）
 * @param v 要转换的值（0-65535）
 * @return 指向dest的指针
 */
const char *u8x8_u16toap(char * dest, uint16_t v)
{
  uint8_t pos;
  uint8_t d;
  uint16_t c;
  c = 10000;                           /* 从万位开始 */
  for( pos = 0; pos < 5; pos++ )
  {
      d = '0';                         /* 初始化为'0' */
      while( v >= c )
      {
	v -= c;                        /* 减去除数 */
	d++;                           /* 数字字符+1 */
      }
      dest[pos] = d;                   /* 存储当前位的字符 */
      c /= 10;                         /* 除数缩小10倍 */
  }
  dest[5] = '\0';                      /* 添加字符串结束符 */
  return dest;
}

/**
 * @brief 将16位无符号整数转换为指定位数的字符串
 *
 * @param v 要转换的值（0-65535）
 * @param d 需要的数字位数（1-5）
 * @return 指向静态缓冲区中结果字符串的指针（跳过前导零）
 */
const char *u8x8_u16toa(uint16_t v, uint8_t d)
{
  static char buf[6];
  d = 5-d;                             /* 计算要跳过的前导零数量 */
  return u8x8_u16toap(buf, v) + d;     /* 返回跳过前导零后的字符串 */
}

/**
 * @brief 将16位无符号整数转换为无前导零的字符串
 *
 * 自动去除前导零，只保留有效数字。
 * 如果值为0，返回"0"。
 *
 * @param v 要转换的值（0-65535）
 * @return 指向静态缓冲区中结果字符串的指针
 */
const char *u8x8_utoa(uint16_t v)
{
  const char *s = u8x8_u16toa(v, 5);  /* 转换为5位字符串 */
  while( *s == '0' )
    s++;                               /* 跳过前导零 */
  if ( *s == '\0' )
    s--;                               /* 如果全是0，返回最后一个'0' */
  return s;
}
