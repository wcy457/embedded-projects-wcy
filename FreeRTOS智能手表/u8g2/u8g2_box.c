/**
 * @file u8g2_box.c
 * @brief u8g2库的矩形绘制功能实现文件
 *
 * 本文件提供了在u8g2图形库中绘制矩形相关图形的功能，包括：
 * - 绘制填充矩形（Box）
 * - 绘制空心矩形边框（Frame）
 * - 绘制圆角填充矩形（RBox）
 * - 绘制圆角空心矩形边框（RFrame）
 *
 * 这些是GUI界面中最基础的图形元素，广泛用于按钮、面板、进度条等控件的绘制。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 * Copyright (c) 2016, olikraus@gmail.com
 */

#include "u8g2.h"

/**
 * @brief 绘制填充矩形
 * @param u8g2  u8g2显示结构体指针
 * @param x     矩形左上角X坐标
 * @param y     矩形左上角Y坐标
 * @param w     矩形宽度（像素）
 * @param h     矩形高度（像素）
 *
 * @note 限制：w和h不能为0
 * 通过逐行绘制水平线来实现矩形填充。
 */
void u8g2_DrawBox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h)
{
#ifdef U8G2_WITH_INTERSECTION
  /* 检查矩形区域是否与屏幕可见区域相交 */
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */
  while( h != 0 )
  {
    u8g2_DrawHVLine(u8g2, x, y, w, 0);  // 绘制一行水平线
    y++;     // 移动到下一行
    h--;     // 剩余高度递减
  }
}


/**
 * @brief 绘制空心矩形边框
 * @param u8g2  u8g2显示结构体指针
 * @param x     矩形左上角X坐标
 * @param y     矩形左上角Y坐标
 * @param w     矩形宽度（像素）
 * @param h     矩形高度（像素）
 *
 * @note 限制：w和h不能为0
 * 只绘制矩形的四条边，内部不填充。
 */
void u8g2_DrawFrame(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h)
{
  u8g2_uint_t xtmp = x;   // 保存左上角X坐标，用于绘制底边

#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  u8g2_DrawHVLine(u8g2, x, y, w, 0);       // 绘制顶边（水平线）
  if (h >= 2) {
    h-=2;                                    // 减去上下两条边的高度
    y++;                                     // 移动到左边框起始位置
    if (h > 0) {
      u8g2_DrawHVLine(u8g2, x, y, h, 1);    // 绘制左边框（垂直线）
      x+=w;
      x--;                                   // 移动到右边框位置
      u8g2_DrawHVLine(u8g2, x, y, h, 1);    // 绘制右边框（垂直线）
      y+=h;                                  // 移动到底边位置
    }
    u8g2_DrawHVLine(u8g2, xtmp, y, w, 0);   // 绘制底边（水平线）
  }
}




/**
 * @brief 绘制圆角填充矩形
 * @param u8g2  u8g2显示结构体指针
 * @param x     矩形左上角X坐标
 * @param y     矩形左上角Y坐标
 * @param w     矩形宽度（像素）
 * @param h     矩形高度（像素）
 * @param r     圆角半径（像素）
 *
 * 绘制算法：
 * 1. 在四个角绘制四分之一填充圆（Disc）
 * 2. 用填充矩形连接各角之间的区域
 */
void u8g2_DrawRBox(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h, u8g2_uint_t r)
{
  u8g2_uint_t xl, yu;   // 左上角圆弧中心坐标
  u8g2_uint_t yl, xr;   // 右下角圆弧中心坐标

#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  /* 计算四个圆弧中心的坐标 */
  xl = x;
  xl += r;         // 左侧圆弧中心X
  yu = y;
  yu += r;         // 上侧圆弧中心Y

  xr = x;
  xr += w;
  xr -= r;
  xr -= 1;         // 右侧圆弧中心X

  yl = y;
  yl += h;
  yl -= r;
  yl -= 1;         // 下侧圆弧中心Y

  /* 绘制四个角的四分之一填充圆 */
  u8g2_DrawDisc(u8g2, xl, yu, r, U8G2_DRAW_UPPER_LEFT);    // 左上角
  u8g2_DrawDisc(u8g2, xr, yu, r, U8G2_DRAW_UPPER_RIGHT);   // 右上角
  u8g2_DrawDisc(u8g2, xl, yl, r, U8G2_DRAW_LOWER_LEFT);    // 左下角
  u8g2_DrawDisc(u8g2, xr, yl, r, U8G2_DRAW_LOWER_RIGHT);   // 右下角

  {
    u8g2_uint_t ww, hh;

    ww = w;
    ww -= r;
    ww -= r;         // 计算中间区域宽度
    xl++;
    yu++;

    if ( ww >= 3 )
    {
      ww -= 2;
      u8g2_DrawBox(u8g2, xl, y, ww, r+1);    // 填充顶部中间区域
      u8g2_DrawBox(u8g2, xl, yl, ww, r+1);   // 填充底部中间区域
    }

    hh = h;
    hh -= r;
    hh -= r;         // 计算中间区域高度
    if ( hh >= 3 )
    {
      hh -= 2;
      u8g2_DrawBox(u8g2, x, yu, w, hh);      // 填充中央区域
    }
  }
}


/**
 * @brief 绘制圆角空心矩形边框
 * @param u8g2  u8g2显示结构体指针
 * @param x     矩形左上角X坐标
 * @param y     矩形左上角Y坐标
 * @param w     矩形宽度（像素）
 * @param h     矩形高度（像素）
 * @param r     圆角半径（像素）
 *
 * 绘制算法：
 * 1. 在四个角绘制四分之一空心圆（Circle）
 * 2. 用直线连接各角之间的边
 */
void u8g2_DrawRFrame(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t h, u8g2_uint_t r)
{
  u8g2_uint_t xl, yu;   // 左上角圆弧中心坐标

#ifdef U8G2_WITH_INTERSECTION
  if ( u8g2_IsIntersection(u8g2, x, y, x+w, y+h) == 0 )
    return;
#endif /* U8G2_WITH_INTERSECTION */

  xl = x;
  xl += r;         // 左侧圆弧中心X
  yu = y;
  yu += r;         // 上侧圆弧中心Y

  {
    u8g2_uint_t yl, xr;

    xr = x;
    xr += w;
    xr -= r;
    xr -= 1;       // 右侧圆弧中心X

    yl = y;
    yl += h;
    yl -= r;
    yl -= 1;       // 下侧圆弧中心Y

    /* 绘制四个角的四分之一空心圆 */
    u8g2_DrawCircle(u8g2, xl, yu, r, U8G2_DRAW_UPPER_LEFT);    // 左上角
    u8g2_DrawCircle(u8g2, xr, yu, r, U8G2_DRAW_UPPER_RIGHT);   // 右上角
    u8g2_DrawCircle(u8g2, xl, yl, r, U8G2_DRAW_LOWER_LEFT);    // 左下角
    u8g2_DrawCircle(u8g2, xr, yl, r, U8G2_DRAW_LOWER_RIGHT);   // 右下角
  }

  {
    u8g2_uint_t ww, hh;

    ww = w;
    ww -= r;
    ww -= r;       // 计算水平边中间段的长度
    hh = h;
    hh -= r;
    hh -= r;       // 计算垂直边中间段的长度

    xl++;
    yu++;

    if ( ww >= 3 )
    {
      ww -= 2;
      h--;
      u8g2_DrawHLine(u8g2, xl, y, ww);      // 绘制顶边中间段
      u8g2_DrawHLine(u8g2, xl, y+h, ww);    // 绘制底边中间段
    }

    if ( hh >= 3 )
    {
      hh -= 2;
      w--;
      u8g2_DrawVLine(u8g2, x, yu, hh);      // 绘制左边中间段
      u8g2_DrawVLine(u8g2, x+w, yu, hh);    // 绘制右边中间段
    }
  }
}

