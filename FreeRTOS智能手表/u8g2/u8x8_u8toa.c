/**
 * @file u8x8_u8toa.c
 * @brief 8位无符号整数转字符串功能
 *
 * 本文件提供了将8位无符号整数（0-255）转换为字符串的功能。
 * 输出始终为3位数字，不足位补零。
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

/** 百位、十位、个位的除数表 */
static const unsigned char u8x8_u8toa_tab[3]  = { 100, 10, 1 } ;

/**
 * @brief 将8位无符号整数转换为3位字符串（带前导零）
 *
 * 将0-255的值转换为3位ASCII字符串（如"000"、"123"、"255"）。
 * 结果存入dest缓冲区（需要至少4字节）。
 *
 * @param dest 目标缓冲区（至少4字节）
 * @param v 要转换的值（0-255）
 * @return 指向dest的指针
 */
const char *u8x8_u8toap(char * dest, uint8_t v)
{
  uint8_t pos;
  uint8_t d;
  uint8_t c;
  for( pos = 0; pos < 3; pos++ )
  {
      d = '0';                           /* 初始化为'0' */
      c = *(u8x8_u8toa_tab+pos);        /* 获取当前位的除数 */
      while( v >= c )
      {
	v -= c;                          /* 减去除数 */
	d++;                             /* 数字字符+1 */
      }
      dest[pos] = d;                     /* 存储当前位的字符 */
  }
  dest[3] = '\0';                        /* 添加字符串结束符 */
  return dest;
}

/**
 * @brief 将8位无符号整数转换为指定位数的字符串
 *
 * @param v 要转换的值（0-255）
 * @param d 需要的数字位数（1-3）
 * @return 指向静态缓冲区中结果字符串的指针（跳过前导零）
 */
const char *u8x8_u8toa(uint8_t v, uint8_t d)
{
  static char buf[4];
  d = 3-d;                               /* 计算要跳过的前导零数量 */
  return u8x8_u8toap(buf, v) + d;        /* 返回跳过前导零后的字符串 */
}

