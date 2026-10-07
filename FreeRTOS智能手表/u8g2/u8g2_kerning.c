/**
 * @file u8g2_kerning.c
 * @brief u8g2库的字体字距调整功能实现文件
 *
 * 本文件实现了u8g2图形库中字体字距调整（Kerning）功能。
 * 字距调整用于优化相邻字符之间的间距，使文本显示更加美观。
 *
 * 字距调整的工作原理：
 *   1. 查找第一个字符（e1）在字距表中的索引
 *   2. 查找与第二个字符（e2）组成的字符对的字距值
 *   3. 返回字距调整值（正值增加间距，负值减少间距）
 *
 * 支持两种字距表格式：
 *   1. 结构化格式（u8g2_kerning_t）：使用两级索引表，适合大型字体
 *   2. 简单表格格式：使用三元组数组，适合小型字体
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
 */

#include "u8g2.h"

/**
 * @brief 空字距调整函数（返回0）
 *
 * 当字体不支持字距调整时使用的默认函数。
 * 该函数作为"u8g2_get_kerning_cb"回调使用。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param e1    第一个字符的编码
 * @param e2    第二个字符的编码
 * @return      始终返回0（无字距调整）
 */
/*
uint8_t u8g2_GetNullKerning(u8g2_t *u8g2, uint16_t e1, uint16_t e2)
{
  return 0;
}
*/

/**
 * @brief 获取两个字符之间的字距调整值（结构化格式）
 *
 * 使用两级索引表查找字距调整值。
 * 第一级表查找第一个字符，第二级表查找与第二个字符的配对。
 * 该函数作为"u8g2_get_kerning_cb"回调使用。
 *
 * @param u8g2    u8g2显示结构体指针（未使用）
 * @param kerning 字距调整表结构体指针
 * @param e1      第一个字符的编码
 * @param e2      第二个字符的编码
 * @return        字距调整值，未找到则返回0
 */
uint8_t u8g2_GetKerning(U8X8_UNUSED u8g2_t *u8g2, u8g2_kerning_t *kerning, uint16_t e1, uint16_t e2)
{
  uint16_t i1, i2, cnt, end;
  if ( kerning == NULL )    /* 字距表为空，返回0 */
    return 0;

  /* 第一步：在第一级表中查找字符e1 */
  cnt = kerning->first_table_cnt;
  cnt--;	/* 忽略最后一个元素（0x0ffff结束标记） */
  for( i1 = 0; i1 < cnt; i1++ )
  {
    if ( kerning->first_encoding_table[i1] == e1 )  /* 找到匹配的编码 */
      break;
  }
  if ( i1 >= cnt )
    return 0;	/* e1不在字距表中，返回0 */

  /* 第二步：在第二级表中查找字符e2 */
  /* 获取第二级表的结束索引 */
  end = kerning->index_to_second_table[i1+1];
  /* 在第二级表范围内查找e2 */
  for( i2 = kerning->index_to_second_table[i1]; i2 < end; i2++ )
  {
    if ( kerning->second_encoding_table[i2] == e2 )  /* 找到匹配的编码 */
      break;
  }

  if ( i2 >= end )
    return 0;	/* e2与e1没有配对，返回0 */

  /* 返回字距调整值 */
  return kerning->kerning_values[i2];
}

/**
 * @brief 获取两个字符之间的字距调整值（简单表格格式）
 *
 * 使用三元组数组查找字距调整值。
 * 表格格式：[e1, e2, kerning_value, e1, e2, kerning_value, ..., 0xffff]
 * 以0xffff作为结束标记。
 *
 * @param u8g2  u8g2显示结构体指针（未使用）
 * @param kt    字距调整表指针（三元组数组）
 * @param e1    第一个字符的编码
 * @param e2    第二个字符的编码
 * @return      字距调整值，未找到则返回0
 */
uint8_t u8g2_GetKerningByTable(U8X8_UNUSED u8g2_t *u8g2, const uint16_t *kt, uint16_t e1, uint16_t e2)
{
  uint16_t i;
  i = 0;
  if ( kt == NULL )    /* 字距表为空，返回0 */
    return 0;
  /* 遍历三元组数组查找匹配的字符对 */
  for(;;)
  {
    if ( kt[i] == 0x0ffff )    /* 遇到结束标记，停止查找 */
      break;
    if ( kt[i] == e1 && kt[i+1] == e2 )  /* 找到匹配的字符对 */
      return kt[i+2];          /* 返回字距调整值 */
    i+=3;                      /* 移动到下一个三元组 */
  }
  return 0;  /* 未找到匹配的字符对 */
}

