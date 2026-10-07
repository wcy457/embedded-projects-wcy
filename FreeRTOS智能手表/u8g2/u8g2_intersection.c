/**
 * @file u8g2_intersection.c
 * @brief u8g2库的矩形相交计算功能实现文件
 *
 * 本文件实现了u8g2图形库中矩形区域相交判断功能。
 * 用于裁剪操作，判断绘制对象是否在可见区域内。
 *
 * 相交判断算法：
 *   使用决策树方法判断两个一维区间是否有交集。
 *   通过两次一维相交判断（x方向和y方向）来判断二维矩形是否相交。
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

/** 强制内联宏定义（GCC编译器） */
#ifdef __GNUC__
#define U8G2_ALWAYS_INLINE __inline__ __attribute__((always_inline))
#else
#define U8G2_ALWAYS_INLINE
#endif


#if defined(U8G2_WITH_INTERSECTION) || defined(U8G2_WITH_CLIP_WINDOW_SUPPORT)

#ifdef OLD_VERSION_WITH_SYMETRIC_BOUNDARIES

/*
  相交判断的假设条件：
    a1 <= a2 始终成立

    最小化版本的真值表：
    ---1----0 1             b1 <= a2 && b1 > b2
    -----1--0 1             b2 >= a1 && b1 > b2
    ---1-1--- 1             b1 <= a2 && b2 >= a1
  */


/**
 * @brief 判断两个一维区间是否相交（对称边界版本，旧版）
 *
 * 使用决策树方法判断区间[a0, a1]和[v0, v1]是否有交集。
 * 边界值v1和a1包含在区间内。
 *
 * @param a0    第一个区间的起始位置
 * @param a1    第一个区间的结束位置（包含）
 * @param v0    第二个区间的起始位置
 * @param v1    第二个区间的结束位置（包含）
 * @return      1表示有交集，0表示无交集
 *
 * 测试用例：
 *   u8g2_is_intersection_decision_tree(4, 6, 7, 9) == 0  // 无交集
 *   u8g2_is_intersection_decision_tree(4, 6, 6, 9) != 0  // 有交集（边界重叠）
 *   u8g2_is_intersection_decision_tree(6, 9, 4, 6) != 0  // 有交集
 *   u8g2_is_intersection_decision_tree(7, 9, 4, 6) == 0  // 无交集
 */

//static uint8_t U8G2_ALWAYS_INLINE u8g2_is_intersection_decision_tree(u8g_uint_t a0, u8g_uint_t a1, u8g_uint_t v0, u8g_uint_t v1)
static uint8_t u8g2_is_intersection_decision_tree(u8g2_uint_t a0, u8g2_uint_t a1, u8g2_uint_t v0, u8g2_uint_t v1)
{
  if ( v0 <= a1 )      /* v0在a1左侧或重叠 */
  {
    if ( v1 >= a0 )    /* v1在a0右侧或重叠 */
    {
      return 1;        /* 有交集 */
    }
    else
    {
      if ( v0 > v1 )   /* v0 > v1表示区间环绕 */
      {
	return 1;      /* 环绕情况下有交集 */
      }
      else
      {
	return 0;      /* 无交集 */
      }
    }
  }
  else
  {
    if ( v1 >= a0 )    /* v1在a0右侧或重叠 */
    {
      if ( v0 > v1 )   /* v0 > v1表示区间环绕 */
      {
	return 1;      /* 环绕情况下有交集 */
      }
      else
      {
	return 0;      /* 无交集 */
      }
    }
    else
    {
      return 0;        /* 无交集 */
    }
  }
}

#endif	/* OLD_VERSION_WITH_SYMETRIC_BOUNDARIES */


/**
 * @brief 判断两个一维区间是否相交（非对称边界版本）
 *
 * 使用决策树方法判断区间[a0, a1)和[v0, v1)是否有交集。
 * 注意：上界a1和v1不包含在区间内（非对称边界）。
 * 不支持v0 == v1的情况（会返回1）。
 *
 * @param a0    第一个区间的起始位置（包含）
 * @param a1    第一个区间的结束位置（不包含）
 * @param v0    第二个区间的起始位置（包含）
 * @param v1    第二个区间的结束位置（不包含）
 * @return      1表示有交集，0表示无交集
 */
uint8_t u8g2_is_intersection_decision_tree(u8g2_uint_t a0, u8g2_uint_t a1, u8g2_uint_t v0, u8g2_uint_t v1)
{
  if ( v0 < a1 )		/* v0在a1左侧（不包含边界） */
  {
    if ( v1 > a0 )	/* v1在a0右侧（不包含边界） */
    {
      return 1;        /* 有交集 */
    }
    else
    {
      if ( v0 > v1 )	/* v0 > v1表示区间环绕 */
      {
	return 1;      /* 环绕情况下有交集 */
      }
      else
      {
	return 0;      /* 无交集 */
      }
    }
  }
  else
  {
    if ( v1 > a0 )	/* v1在a0右侧（不包含边界） */
    {
      if ( v0 > v1 )	/* v0 > v1表示区间环绕 */
      {
	return 1;      /* 环绕情况下有交集 */
      }
      else
      {
	return 0;      /* 无交集 */
      }
    }
    else
    {
      return 0;        /* 无交集 */
    }
  }
}



/**
 * @brief 判断矩形区域是否与用户窗口相交
 *
 * 检查给定的矩形区域[x0, y0, x1, y1]是否与u8g2的用户裁剪窗口相交。
 * 上界不包含在内（非对称边界）。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x0    矩形左上角x坐标
 * @param y0    矩形左上角y坐标
 * @param x1    矩形右下角x坐标（不包含）
 * @param y1    矩形右下角y坐标（不包含）
 * @return      1表示有交集，0表示无交集
 */
/* 上界不包含（非对称边界） */
uint8_t u8g2_IsIntersection(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t x1, u8g2_uint_t y1)
{
  /* 首先检查y方向是否有交集 */
  if ( u8g2_is_intersection_decision_tree(u8g2->user_y0, u8g2->user_y1, y0, y1) == 0 )
    return 0;

  /* y方向有交集，再检查x方向 */
  return u8g2_is_intersection_decision_tree(u8g2->user_x0, u8g2->user_x1, x0, x1);
}


#endif /* U8G2_WITH_INTERSECTION */

