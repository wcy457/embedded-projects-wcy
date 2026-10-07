/**
 * @file u8g2_hvline.c
 * @brief u8g2库的水平/垂直线绘制功能实现文件
 *
 * 本文件实现了u8g2图形库中水平线和垂直线的绘制功能，
 * 包括线段裁剪、坐标转换和实际绘制操作。
 *
 * 调用关系:
 *   u8g2_DrawHVLine()          - 用户层接口，支持4个方向
 *     -> u8g2->cb->draw_l90()  - 回调函数，处理显示旋转
 *       -> u8g2_draw_hv_line_2dir() - 内部函数，2方向绘制
 *         -> u8g2->ll_hvline()  - 底层硬件绘制函数
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
 * 调用树 (Calltree)
 *
 *   void u8g2_DrawHVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
 *   u8g2->cb->draw_l90
 *   u8g2_draw_hv_line_2dir
 *   u8g2->ll_hvline(u8g2, x, y, len, dir);
 *
 *
 */

#include "u8g2.h"
#include <assert.h>

/*==========================================================*/
/* 裁剪相交计算过程 */

/**
 * @brief 线段裁剪函数 - 将线段裁剪到指定范围内
 *
 * 将从位置a开始、长度为len的线段裁剪到[c, d)范围内。
 * 该函数用于确保绘制的线段不超出显示区域或裁剪窗口。
 *
 * @param ap    指向线段起始位置的指针，裁剪后会被更新
 * @param len   指向线段长度的指针，裁剪后会被更新
 * @param c     裁剪范围的起始位置（包含）
 * @param d     裁剪范围的结束位置（不包含）
 * @return      1表示有交集（线段可见），0表示无交集（线段被完全裁剪）
 *
 * @note 假设条件: len > 0, c <= d（不检查c<=d）
 * @note 当a > b时也会正确处理，避免内存损坏
 */
static uint8_t u8g2_clip_intersection2(u8g2_uint_t *ap, u8g2_uint_t *len, u8g2_uint_t c, u8g2_uint_t d)
{
  u8g2_uint_t a = *ap;   /* 获取线段起始位置 */
  u8g2_uint_t b;
  b  = a;
  b += *len;             /* 计算线段结束位置 (a+len) */

  /*
    Description:
      clip range from a (included) to b (excluded) agains c (included) to d (excluded)
    Assumptions:
      a <= b		(violation is checked and handled correctly)
      c <= d		(this is not checked)
    will return 0 if there is no intersection and if a > b

    optimized clipping: c is set to 0 --> 27 Oct 2018: again removed the c==0 assumption
    
    replaced by uint8_t u8g2_clip_intersection2
  */

  /* handle the a>b case correctly. If code and time is critical, this could */
  /* be removed completly (be aware about memory curruption for wrong */
  /* arguments) or return 0 for a>b (will lead to skipped lines for wrong */
  /* arguments) */  
  
  /* 处理a > b的异常情况，避免内存损坏 */
  /* 注意：完全删除此if块可能导致内存损坏 */
  if ( a > b )
  {
    /* 不能简单返回0，需要处理负数a的情况 */
    if ( a < d )
    {
      b = d;
      b--;    /* b = d - 1，确保b在有效范围内 */
    }
    else
    {
      a = c;  /* 将a重置为裁剪范围起始 */
    }
  }
  
  /* 从现在起，假设 a <= b 成立 */

  /* 以下进行实际的裁剪计算 */
  if ( a >= d )      /* 线段完全在裁剪范围右侧 */
    return 0;
  if ( b <= c )      /* 线段完全在裁剪范围左侧 */
    return 0;
  if ( a < c )       /* 线段左端超出范围，向右裁剪 */
    a = c;
  if ( b > d )       /* 线段右端超出范围，向左裁剪 */
    b = d;

  *ap = a;           /* 更新起始位置 */
  b -= a;            /* 计算裁剪后的长度 */
  *len = b;          /* 更新长度 */
  return 1;          /* 返回1表示有交集 */
}



/*==========================================================*/
/* 绘制过程函数 */

/**
 * @brief 内部绘制函数 - 绘制水平或垂直线段（2方向版本）
 *
 * 该函数将y坐标转换为像素缓冲区的本地坐标，然后调用底层绘制函数。
 * 在调用此函数之前，应该已经完成了裁剪操作。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标
 * @param y     线段起始点的y坐标
 * @param len   线段长度（像素），不能为0
 * @param dir   方向: 0=水平线（从左到右），1=垂直线（从上到下）
 *
 * @note 裁剪在显示旋转之前完成
 * @note 此函数调整y坐标到本地缓冲区坐标系
 */
void u8g2_draw_hv_line_2dir(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{

  /* 裁剪在显示旋转之前完成 */

  /* 将全局y坐标转换为像素缓冲区的本地坐标 */
  /* pixel_curr_row是当前像素缓冲区对应的起始行 */
  y -= u8g2->pixel_curr_row;

  /* 调用底层硬件绘制函数 */
  u8g2->ll_hvline(u8g2, x, y, len, dir);
}


/**
 * @brief 绘制水平/垂直线段 - 用户层接口函数
 *
 * 这是水平/垂直线绘制的顶层函数，用户应该调用此函数。
 * 支持4个方向的线段绘制，内部处理裁剪和旋转。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标
 * @param y     线段起始点的y坐标
 * @param len   线段长度（像素），可以为0（不绘制）
 * @param dir   方向: 0=从左到右, 1=从上到下, 2=从右到左, 3=从下到上
 */
void u8g2_DrawHVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  /* 调用回调函数（如u8g2_draw_l90_r0） */
  /* 回调函数可能会旋转水平/垂直线 */
  /* 旋转后会调用u8g2_draw_hv_line_4dir() */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
  /* 检查页面裁剪窗口是否有交集 */
  if ( u8g2->is_page_clip_window_intersection != 0 )
#endif /* U8G2_WITH_CLIP_WINDOW_SUPPORT */
    if ( len != 0 )    /* 长度为0时不绘制 */
    {

      /* 将4方向转换为2方向 */
      if ( len > 1 )
      {
	if ( dir == 2 )    /* 从右到左：调整起始x坐标 */
	{
	  x -= len;
	  x++;
	}
	else if ( dir == 3 )  /* 从下到上：调整起始y坐标 */
	{
	  y -= len;
	  y++;
	}
      }
      dir &= 1;    /* 将方向限制为0或1（水平或垂直） */

      /* 针对用户窗口进行裁剪 */
      if ( dir == 0 )    /* 水平线 */
      {
	if ( y < u8g2->user_y0 )    /* 检查y是否在用户窗口内 */
	  return;
	if ( y >= u8g2->user_y1 )
	  return;
	/* 对x方向进行裁剪 */
	if ( u8g2_clip_intersection2(&x, &len, u8g2->user_x0, u8g2->user_x1) == 0 )
	  return;
      }
      else    /* 垂直线 */
      {
	if ( x < u8g2->user_x0 )    /* 检查x是否在用户窗口内 */
	  return;
	if ( x >= u8g2->user_x1 )
	  return;
	/* 对y方向进行裁剪 */
	if ( u8g2_clip_intersection2(&y, &len, u8g2->user_y0, u8g2->user_y1) == 0 )
	  return;
      }

      /* 调用回调函数执行实际绘制 */
      u8g2->cb->draw_l90(u8g2, x, y, len, dir);
    }
}

/**
 * @brief 绘制水平线段
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标
 * @param y     线段起始点的y坐标
 * @param len   线段长度（像素）
 */
void u8g2_DrawHLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len)
{
// #ifdef U8G2_WITH_INTERSECTION
//   if ( u8g2_IsIntersection(u8g2, x, y, x+len, y+1) == 0 )
//     return;
// #endif /* U8G2_WITH_INTERSECTION */
  u8g2_DrawHVLine(u8g2, x, y, len, 0);  /* 方向0=水平线 */
}

/**
 * @brief 绘制垂直线段
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     线段起始点的x坐标
 * @param y     线段起始点的y坐标
 * @param len   线段长度（像素）
 */
void u8g2_DrawVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len)
{
// #ifdef U8G2_WITH_INTERSECTION
//   if ( u8g2_IsIntersection(u8g2, x, y, x+1, y+len) == 0 )
//     return;
// #endif /* U8G2_WITH_INTERSECTION */
  u8g2_DrawHVLine(u8g2, x, y, len, 1);  /* 方向1=垂直线 */
}

/**
 * @brief 绘制单个像素点
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x     像素点的x坐标
 * @param y     像素点的y坐标
 */
void u8g2_DrawPixel(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y)
{
#ifdef U8G2_WITH_INTERSECTION
  /* 检查像素是否在用户窗口内 */
  if ( y < u8g2->user_y0 )
    return;
  if ( y >= u8g2->user_y1 )
    return;
  if ( x < u8g2->user_x0 )
    return;
  if ( x >= u8g2->user_x1 )
    return;
#endif /* U8G2_WITH_INTERSECTION */
  /* 绘制长度为1的水平线来实现单像素绘制 */
  u8g2_DrawHVLine(u8g2, x, y, 1, 0);
}

/**
 * @brief 设置绘图颜色
 *
 * 为所有绘图函数设置绘图颜色。
 * 颜色值可以是0、1或2：
 *   - color=1: 设置显示内存为1。对于OLED通常意味着点亮像素；
 *     对于LCD通常意味着使能LCD段（吸收光线）；对于电子墨水屏意味着黑色
 *   - color=0: 清除像素（通常为背景色）
 *   - color=2: XOR异或操作（2017年1月7日添加）
 *
 * @param u8g2   u8g2显示结构体指针
 * @param color  绘图颜色值（0、1或2）
 */
void u8g2_SetDrawColor(u8g2_t *u8g2, uint8_t color)
{
  u8g2->draw_color = color;	/* 设置绘图颜色 */
  if ( color >= 3 )             /* 颜色值无效时默认设为1 */
    u8g2->draw_color = 1;
}

