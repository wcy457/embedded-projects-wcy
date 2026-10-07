/**
 * @file u8g2_line.c
 * @brief u8g2库的任意角度线段绘制功能实现文件
 *
 * 本文件实现了u8g2图形库中任意角度线段的绘制功能，
 * 使用Bresenham直线算法实现高效的线段绘制。
 *
 * Bresenham算法是一种光栅化算法，通过整数运算来确定
 * 最接近理想直线的像素点，避免了浮点运算，适合嵌入式系统。
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
 * @brief 绘制任意角度的线段
 *
 * 使用Bresenham直线算法绘制从(x1,y1)到(x2,y2)的线段。
 * 该算法通过整数加法和比较来确定每个像素位置，避免浮点运算。
 *
 * 算法原理：
 * 1. 计算x和y方向的增量dx和dy
 * 2. 如果dy>dx，则交换x和y坐标（将陡峭线转为平缓线处理）
 * 3. 确保x1<x2（从左到右绘制）
 * 4. 使用误差项err决定y方向何时需要步进
 * 5. 每绘制一个像素，误差减小dy；当误差<0时，y步进并补偿误差
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x1    线段起点的x坐标
 * @param y1    线段起点的y坐标
 * @param x2    线段终点的x坐标
 * @param y2    线段终点的y坐标
 *
 * @note 当前没有进行裁剪检查（应该添加）
 */
void u8g2_DrawLine(u8g2_t *u8g2, u8g2_uint_t x1, u8g2_uint_t y1, u8g2_uint_t x2, u8g2_uint_t y2)
{
  u8g2_uint_t tmp;
  u8g2_uint_t x,y;           /* 当前绘制点坐标 */
  u8g2_uint_t dx, dy;        /* x和y方向的增量 */
  u8g2_int_t err;            /* 误差项，用于决定y方向步进时机 */
  u8g2_int_t ystep;          /* y方向步进值（+1或-1） */

  uint8_t swapxy = 0;        /* 标志：是否交换了x和y坐标 */

  /* 当前没有进行裁剪检查，应该添加... */

  /* 计算x和y方向的绝对增量 */
  if ( x1 > x2 ) dx = x1-x2; else dx = x2-x1;
  if ( y1 > y2 ) dy = y1-y2; else dy = y2-y1;

  /* 如果dy > dx，说明线段更陡峭，交换x和y使其变为平缓线 */
  if ( dy > dx )
  {
    swapxy = 1;              /* 设置交换标志 */
    tmp = dx; dx =dy; dy = tmp;     /* 交换dx和dy */
    tmp = x1; x1 =y1; y1 = tmp;     /* 交换起点坐标 */
    tmp = x2; x2 =y2; y2 = tmp;     /* 交换终点坐标 */
  }
  /* 确保从左到右绘制（x1 < x2） */
  if ( x1 > x2 )
  {
    tmp = x1; x1 =x2; x2 = tmp;     /* 交换起点和终点 */
    tmp = y1; y1 =y2; y2 = tmp;
  }

  /* 初始化误差项（dx的一半） */
  err = dx >> 1;
  /* 确定y方向的步进方向 */
  if ( y2 > y1 ) ystep = 1; else ystep = -1;
  y = y1;

  /* 处理x2等于最大值的情况，避免溢出 */
#ifndef  U8G2_16BIT
  if ( x2 == 255 )           /* 8位坐标最大值 */
    x2--;
#else
  if ( x2 == 0xffff )        /* 16位坐标最大值 */
    x2--;
#endif

  /* Bresenham主循环：逐像素绘制线段 */
  for( x = x1; x <= x2; x++ )
  {
    if ( swapxy == 0 )
      u8g2_DrawPixel(u8g2, x, y);    /* 正常绘制 */
    else
      u8g2_DrawPixel(u8g2, y, x);    /* 交换了坐标，需要交换回来 */
    err -= (u8g2_uint_t)dy;          /* 误差减小 */
    if ( err < 0 )                    /* 误差小于0，需要y方向步进 */
    {
      y += (u8g2_uint_t)ystep;       /* y方向步进 */
      err += (u8g2_uint_t)dx;        /* 补偿误差 */
    }
  }
}

