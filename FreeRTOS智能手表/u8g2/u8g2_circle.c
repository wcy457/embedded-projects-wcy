/**
 * @file u8g2_circle.c
 * @brief u8g2库的圆形和椭圆绘制功能实现文件
 *
 * 本文件提供了在u8g2图形库中绘制圆形和椭圆的功能，包括：
 * - 绘制空心圆（Circle）：使用Bresenham中点圆算法
 * - 绘制填充圆（Disc）：使用垂直线填充
 * - 绘制空心椭圆（Ellipse）：使用中点椭圆算法
 * - 绘制填充椭圆（FilledEllipse）：使用垂直线填充
 *
 * 所有图形都支持选择绘制的象限（上左、上右、下左、下右），
 * 这对于绘制圆角矩形等复合图形非常有用。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 * Copyright (c) 2016, olikraus@gmail.com
 */

#include "u8g2.h"

/*==============================================*/
/* 圆形绘制 */

/**
 * @brief 绘制圆的一个象限（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x      当前点的X偏移
 * @param y      当前点的Y偏移
 * @param x0     圆心X坐标
 * @param y0     圆心Y坐标
 * @param option 象限选项（U8G2_DRAW_UPPER_LEFT/UPPER_RIGHT/LOWER_LEFT/LOWER_RIGHT）
 *
 * 利用圆的八对称性，根据option参数绘制指定象限的像素点。
 * 对于每个(x,y)，会绘制两个点（利用对称性）。
 */
static void u8g2_draw_circle_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option) U8G2_NOINLINE;

static void u8g2_draw_circle_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option)
{
    /* 右上象限 */
    if ( option & U8G2_DRAW_UPPER_RIGHT )
    {
      u8g2_DrawPixel(u8g2, x0 + x, y0 - y);
      u8g2_DrawPixel(u8g2, x0 + y, y0 - x);
    }

    /* 左上象限 */
    if ( option & U8G2_DRAW_UPPER_LEFT )
    {
      u8g2_DrawPixel(u8g2, x0 - x, y0 - y);
      u8g2_DrawPixel(u8g2, x0 - y, y0 - x);
    }

    /* 右下象限 */
    if ( option & U8G2_DRAW_LOWER_RIGHT )
    {
      u8g2_DrawPixel(u8g2, x0 + x, y0 + y);
      u8g2_DrawPixel(u8g2, x0 + y, y0 + x);
    }

    /* 左下象限 */
    if ( option & U8G2_DRAW_LOWER_LEFT )
    {
      u8g2_DrawPixel(u8g2, x0 - x, y0 + y);
      u8g2_DrawPixel(u8g2, x0 - y, y0 + x);
    }
}

/**
 * @brief 使用Bresenham中点圆算法绘制圆（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x0     圆心X坐标
 * @param y0     圆心Y坐标
 * @param rad    圆的半径
 * @param option 象限选项
 *
 * 使用Bresenham中点圆算法，只需计算1/8圆弧，利用对称性绘制其余部分。
 * 算法特点：只使用整数运算，避免浮点运算，适合嵌入式系统。
 */
static void u8g2_draw_circle(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rad, uint8_t option)
{
    u8g2_int_t f;          // 决策参数
    u8g2_int_t ddF_x;      // x方向的增量
    u8g2_int_t ddF_y;      // y方向的增量
    u8g2_uint_t x;         // 当前x坐标
    u8g2_uint_t y;         // 当前y坐标

    /* 初始化决策参数和增量 */
    f = 1;
    f -= rad;              // f = 1 - rad
    ddF_x = 1;             // ddF_x = 1
    ddF_y = 0;
    ddF_y -= rad;
    ddF_y *= 2;            // ddF_y = -2 * rad
    x = 0;
    y = rad;               // 从(0, rad)开始

    u8g2_draw_circle_section(u8g2, x, y, x0, y0, option);

    while ( x < y )        // 遍历1/8圆弧
    {
      if (f >= 0)
      {
        y--;               // 选择内侧像素
        ddF_y += 2;
        f += ddF_y;
      }
      x++;                 // 移动到下一个x
      ddF_x += 2;
      f += ddF_x;

      u8g2_draw_circle_section(u8g2, x, y, x0, y0, option);
    }
}

/**
 * @brief 绘制空心圆
 * @param u8g2   u8g2显示结构体指针
 * @param x0     圆心X坐标
 * @param y0     圆心Y坐标
 * @param rad    圆的半径（像素）
 * @param option 象限选项：
 *               - U8G2_DRAW_UPPER_LEFT:  左上象限
 *               - U8G2_DRAW_UPPER_RIGHT: 右上象限
 *               - U8G2_DRAW_LOWER_LEFT:  左下象限
 *               - U8G2_DRAW_LOWER_RIGHT: 右下象限
 *               - 可用OR运算组合多个象限
 */
void u8g2_DrawCircle(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rad, uint8_t option)
{
  /* 检查边界框是否与屏幕相交 */
#ifdef U8G2_WITH_INTERSECTION
  {
    if ( u8g2_IsIntersection(u8g2, x0-rad, y0-rad, x0+rad+1, y0+rad+1) == 0 )
      return;
  }
#endif /* U8G2_WITH_INTERSECTION */


  /* 绘制圆 */
  u8g2_draw_circle(u8g2, x0, y0, rad, option);
}

/*==============================================*/
/* 填充圆（Disc） */

/**
 * @brief 绘制填充圆的一个象限（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x      当前点的X偏移
 * @param y      当前点的Y偏移
 * @param x0     圆心X坐标
 * @param y0     圆心Y坐标
 * @param option 象限选项
 *
 * 与空心圆不同，填充圆使用垂直线(DrawVLine)来填充区域。
 */
static void u8g2_draw_disc_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option) U8G2_NOINLINE;

static void u8g2_draw_disc_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option)
{
    /* 右上象限 */
    if ( option & U8G2_DRAW_UPPER_RIGHT )
    {
      u8g2_DrawVLine(u8g2, x0+x, y0-y, y+1);   // 从上往下画垂直线
      u8g2_DrawVLine(u8g2, x0+y, y0-x, x+1);
    }

    /* 左上象限 */
    if ( option & U8G2_DRAW_UPPER_LEFT )
    {
      u8g2_DrawVLine(u8g2, x0-x, y0-y, y+1);
      u8g2_DrawVLine(u8g2, x0-y, y0-x, x+1);
    }

    /* 右下象限 */
    if ( option & U8G2_DRAW_LOWER_RIGHT )
    {
      u8g2_DrawVLine(u8g2, x0+x, y0, y+1);      // 从圆心往下画垂直线
      u8g2_DrawVLine(u8g2, x0+y, y0, x+1);
    }

    /* 左下象限 */
    if ( option & U8G2_DRAW_LOWER_LEFT )
    {
      u8g2_DrawVLine(u8g2, x0-x, y0, y+1);
      u8g2_DrawVLine(u8g2, x0-y, y0, x+1);
    }
}

/**
 * @brief 使用Bresenham算法绘制填充圆（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x0     圆心X坐标
 * @param y0     圆心Y坐标
 * @param rad    圆的半径
 * @param option 象限选项
 *
 * 算法与空心圆相同，但使用垂直线填充而不是单个像素点。
 */
static void u8g2_draw_disc(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rad, uint8_t option)
{
  u8g2_int_t f;
  u8g2_int_t ddF_x;
  u8g2_int_t ddF_y;
  u8g2_uint_t x;
  u8g2_uint_t y;

  f = 1;
  f -= rad;
  ddF_x = 1;
  ddF_y = 0;
  ddF_y -= rad;
  ddF_y *= 2;
  x = 0;
  y = rad;

  u8g2_draw_disc_section(u8g2, x, y, x0, y0, option);

  while ( x < y )
  {
    if (f >= 0)
    {
      y--;
      ddF_y += 2;
      f += ddF_y;
    }
    x++;
    ddF_x += 2;
    f += ddF_x;

    u8g2_draw_disc_section(u8g2, x, y, x0, y0, option);
  }
}

/**
 * @brief 绘制填充圆
 * @param u8g2   u8g2显示结构体指针
 * @param x0     圆心X坐标
 * @param y0     圆心Y坐标
 * @param rad    圆的半径（像素）
 * @param option 象限选项（同u8g2_DrawCircle）
 */
void u8g2_DrawDisc(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rad, uint8_t option)
{
  /* 检查边界框是否与屏幕相交 */
#ifdef U8G2_WITH_INTERSECTION
  {
    if ( u8g2_IsIntersection(u8g2, x0-rad, y0-rad, x0+rad+1, y0+rad+1) == 0 )
      return;
  }
#endif /* U8G2_WITH_INTERSECTION */

  /* 绘制填充圆 */
  u8g2_draw_disc(u8g2, x0, y0, rad, option);
}

/*==============================================*/
/* 椭圆绘制 */

/**
 * @brief 绘制椭圆的一个象限（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x      当前点的X偏移
 * @param y      当前点的Y偏移
 * @param x0     椭圆中心X坐标
 * @param y0     椭圆中心Y坐标
 * @param option 象限选项
 *
 * 椭圆只有四对称性（不像圆有八对称性），所以每个点只绘制一个像素。
 * 算法参考：Foley, Computer Graphics, p 90
 */
static void u8g2_draw_ellipse_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option) U8G2_NOINLINE;
static void u8g2_draw_ellipse_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option)
{
    /* 右上象限 */
    if ( option & U8G2_DRAW_UPPER_RIGHT )
    {
      u8g2_DrawPixel(u8g2, x0 + x, y0 - y);
    }

    /* 左上象限 */
    if ( option & U8G2_DRAW_UPPER_LEFT )
    {
      u8g2_DrawPixel(u8g2, x0 - x, y0 - y);
    }

    /* 右下象限 */
    if ( option & U8G2_DRAW_LOWER_RIGHT )
    {
      u8g2_DrawPixel(u8g2, x0 + x, y0 + y);
    }

    /* 左下象限 */
    if ( option & U8G2_DRAW_LOWER_LEFT )
    {
      u8g2_DrawPixel(u8g2, x0 - x, y0 + y);
    }
}

/**
 * @brief 使用中点椭圆算法绘制椭圆（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x0     椭圆中心X坐标
 * @param y0     椭圆中心Y坐标
 * @param rx     X轴半径
 * @param ry     Y轴半径
 * @param option 象限选项
 *
 * 中点椭圆算法将椭圆分为两个区域分别处理：
 * 1. 斜率绝对值 < 1 的区域（从(rx,0)开始，y递增）
 * 2. 斜率绝对值 > 1 的区域（从(0,ry)开始，x递增）
 * 使用整数运算，适合嵌入式系统。
 */
static void u8g2_draw_ellipse(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rx, u8g2_uint_t ry, uint8_t option)
{
  u8g2_uint_t x, y;
  u8g2_long_t xchg, ychg;   // 决策参数的增量
  u8g2_long_t err;           // 决策参数
  u8g2_long_t rxrx2;         // 2 * rx^2
  u8g2_long_t ryry2;         // 2 * ry^2
  u8g2_long_t stopx, stopy;  // 终止条件

  /* 计算 2*rx^2 和 2*ry^2 */
  rxrx2 = rx;
  rxrx2 *= rx;
  rxrx2 *= 2;

  ryry2 = ry;
  ryry2 *= ry;
  ryry2 *= 2;

  /* 第一个区域：从(rx, 0)开始，斜率绝对值 < 1 */
  x = rx;
  y = 0;

  xchg = 1;
  xchg -= rx;
  xchg -= rx;
  xchg *= ry;
  xchg *= ry;    // xchg = (1 - 2*rx) * ry^2

  ychg = rx;
  ychg *= rx;    // ychg = rx^2

  err = 0;

  stopx = ryry2;
  stopx *= rx;   // stopx = 2 * ry^2 * rx
  stopy = 0;

  while( stopx >= stopy )
  {
    u8g2_draw_ellipse_section(u8g2, x, y, x0, y0, option);
    y++;
    stopy += rxrx2;
    err += ychg;
    ychg += rxrx2;
    if ( 2*err+xchg > 0 )
    {
      x--;
      stopx -= ryry2;
      err += xchg;
      xchg += ryry2;
    }
  }

  /* 第二个区域：从(0, ry)开始，斜率绝对值 > 1 */
  x = 0;
  y = ry;

  xchg = ry;
  xchg *= ry;    // xchg = ry^2

  ychg = 1;
  ychg -= ry;
  ychg -= ry;
  ychg *= rx;
  ychg *= rx;    // ychg = (1 - 2*ry) * rx^2

  err = 0;

  stopx = 0;

  stopy = rxrx2;
  stopy *= ry;   // stopy = 2 * rx^2 * ry


  while( stopx <= stopy )
  {
    u8g2_draw_ellipse_section(u8g2, x, y, x0, y0, option);
    x++;
    stopx += ryry2;
    err += xchg;
    xchg += ryry2;
    if ( 2*err+ychg > 0 )
    {
      y--;
      stopy -= rxrx2;
      err += ychg;
      ychg += rxrx2;
    }
  }

}

/**
 * @brief 绘制空心椭圆
 * @param u8g2   u8g2显示结构体指针
 * @param x0     椭圆中心X坐标
 * @param y0     椭圆中心Y坐标
 * @param rx     X轴半径（像素）
 * @param ry     Y轴半径（像素）
 * @param option 象限选项（同u8g2_DrawCircle）
 */
void u8g2_DrawEllipse(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rx, u8g2_uint_t ry, uint8_t option)
{
  /* 检查边界框是否与屏幕相交 */
#ifdef U8G2_WITH_INTERSECTION
  {
    if ( u8g2_IsIntersection(u8g2, x0-rx, y0-ry, x0+rx+1, y0+ry+1) == 0 )
      return;
  }
#endif /* U8G2_WITH_INTERSECTION */

  u8g2_draw_ellipse(u8g2, x0, y0, rx, ry, option);
}

/*==============================================*/
/* 填充椭圆 */

/**
 * @brief 绘制填充椭圆的一个象限（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x      当前点的X偏移
 * @param y      当前点的Y偏移
 * @param x0     椭圆中心X坐标
 * @param y0     椭圆中心Y坐标
 * @param option 象限选项
 *
 * 使用垂直线填充椭圆区域。
 */
static void u8g2_draw_filled_ellipse_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option) U8G2_NOINLINE;
static void u8g2_draw_filled_ellipse_section(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t x0, u8g2_uint_t y0, uint8_t option)
{
    /* 右上象限 */
    if ( option & U8G2_DRAW_UPPER_RIGHT )
    {
      u8g2_DrawVLine(u8g2, x0+x, y0-y, y+1);
    }

    /* 左上象限 */
    if ( option & U8G2_DRAW_UPPER_LEFT )
    {
      u8g2_DrawVLine(u8g2, x0-x, y0-y, y+1);
    }

    /* 右下象限 */
    if ( option & U8G2_DRAW_LOWER_RIGHT )
    {
      u8g2_DrawVLine(u8g2, x0+x, y0, y+1);
    }

    /* 左下象限 */
    if ( option & U8G2_DRAW_LOWER_LEFT )
    {
      u8g2_DrawVLine(u8g2, x0-x, y0, y+1);
    }
}

/**
 * @brief 使用中点算法绘制填充椭圆（内部函数）
 * @param u8g2   u8g2显示结构体指针
 * @param x0     椭圆中心X坐标
 * @param y0     椭圆中心Y坐标
 * @param rx     X轴半径
 * @param ry     Y轴半径
 * @param option 象限选项
 *
 * 算法与空心椭圆相同，但使用垂直线填充。
 */
static void u8g2_draw_filled_ellipse(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rx, u8g2_uint_t ry, uint8_t option)
{
  u8g2_uint_t x, y;
  u8g2_long_t xchg, ychg;
  u8g2_long_t err;
  u8g2_long_t rxrx2;
  u8g2_long_t ryry2;
  u8g2_long_t stopx, stopy;

  rxrx2 = rx;
  rxrx2 *= rx;
  rxrx2 *= 2;

  ryry2 = ry;
  ryry2 *= ry;
  ryry2 *= 2;

  x = rx;
  y = 0;

  xchg = 1;
  xchg -= rx;
  xchg -= rx;
  xchg *= ry;
  xchg *= ry;

  ychg = rx;
  ychg *= rx;

  err = 0;

  stopx = ryry2;
  stopx *= rx;
  stopy = 0;

  while( stopx >= stopy )
  {
    u8g2_draw_filled_ellipse_section(u8g2, x, y, x0, y0, option);
    y++;
    stopy += rxrx2;
    err += ychg;
    ychg += rxrx2;
    if ( 2*err+xchg > 0 )
    {
      x--;
      stopx -= ryry2;
      err += xchg;
      xchg += ryry2;
    }
  }

  x = 0;
  y = ry;

  xchg = ry;
  xchg *= ry;

  ychg = 1;
  ychg -= ry;
  ychg -= ry;
  ychg *= rx;
  ychg *= rx;

  err = 0;

  stopx = 0;

  stopy = rxrx2;
  stopy *= ry;


  while( stopx <= stopy )
  {
    u8g2_draw_filled_ellipse_section(u8g2, x, y, x0, y0, option);
    x++;
    stopx += ryry2;
    err += xchg;
    xchg += ryry2;
    if ( 2*err+ychg > 0 )
    {
      y--;
      stopy -= rxrx2;
      err += ychg;
      ychg += rxrx2;
    }
  }

}

/**
 * @brief 绘制填充椭圆
 * @param u8g2   u8g2显示结构体指针
 * @param x0     椭圆中心X坐标
 * @param y0     椭圆中心Y坐标
 * @param rx     X轴半径（像素）
 * @param ry     Y轴半径（像素）
 * @param option 象限选项（同u8g2_DrawCircle）
 */
void u8g2_DrawFilledEllipse(u8g2_t *u8g2, u8g2_uint_t x0, u8g2_uint_t y0, u8g2_uint_t rx, u8g2_uint_t ry, uint8_t option)
{
  /* 检查边界框是否与屏幕相交 */
#ifdef U8G2_WITH_INTERSECTION
  {
    if ( u8g2_IsIntersection(u8g2, x0-rx, y0-ry, x0+rx+1, y0+ry+1) == 0 )
      return;
  }
#endif /* U8G2_WITH_INTERSECTION */

  u8g2_draw_filled_ellipse(u8g2, x0, y0, rx, ry, option);
}


