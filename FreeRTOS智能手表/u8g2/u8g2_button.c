/**
 * @file u8g2_button.c
 * @brief u8g2库的按钮绘制功能实现文件
 *
 * 本文件提供了在u8g2图形库中绘制按钮的功能，包括：
 * - 绘制按钮边框（支持多层边框和阴影）
 * - 绘制带文本的按钮（支持UTF8文本）
 * - 支持按钮的反色显示、水平居中等功能
 *
 * 按钮是GUI界面中最常用的交互控件，适用于STM32智能手表的菜单和设置界面。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 * Copyright (c) 2016, olikraus@gmail.com
 */

#include "u8g2.h"


/**
 * @brief 按钮绘制功能说明
 *
 * 绘制普通或反色文本，支持在文本周围绘制边框。
 * 文本和边框可以水平居中于提供的参考位置。
 * 使用当前的绘制颜色和字体。
 *
 * @note 注意事项：
 * - 不支持绘制颜色2（XOR模式），结果会异常
 * - 此函数会强制设置字体模式为1（透明模式）
 * - 某些字体可能在右侧添加额外间隙（如inb16字体）
 *
 * 按钮高度由当前字体和以下设置决定：
 * - setFontRefHeightText()
 * - setFontRefHeightExtendedText()
 * - setFontRefHeightAll()
 */

/**
 * @brief 按钮标志位说明
 *
 * 边框宽度标志：
 * - U8G2_BTN_BW1 (0x01): 1像素边框
 * - U8G2_BTN_BW2 (0x02): 2像素边框
 * - U8G2_BTN_BW3 (0x03): 3像素边框
 *
 * 阴影标志：
 * - U8G2_BTN_SHADOW0 (0x08): 无阴影偏移
 * - U8G2_BTN_SHADOW1 (0x10): 1像素阴影偏移
 * - U8G2_BTN_SHADOW2 (0x18): 2像素阴影偏移
 *
 * 其他标志：
 * - U8G2_BTN_INV (0x20): 反色显示
 * - U8G2_BTN_HCENTER (0x40): 水平居中
 * - U8G2_BTN_XFRAME (0x80): 额外边框（与阴影不兼容）
 */

/**
 * @brief 绘制按钮边框
 * @param u8g2          u8g2显示结构体指针
 * @param x             文本位置X坐标
 * @param y             文本位置Y坐标
 * @param flags         按钮标志位（边框宽度、阴影、反色、居中等）
 * @param text_width    文本宽度（像素）
 * @param padding_h     水平内边距（像素）
 * @param padding_v     垂直内边距（像素）
 *
 * 按钮总宽度 = text_width + 2*padding_h + 2*border_width
 * 按钮总高度 = 字体高度 + 2*padding_v + 2*border_width
 */
void u8g2_DrawButtonFrame(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t flags, u8g2_uint_t text_width, u8g2_uint_t padding_h, u8g2_uint_t padding_v)
{
  u8g2_uint_t w = text_width;
  
  u8g2_uint_t xx, yy, ww, hh;
  
  u8g2_uint_t gap_frame = U8G2_BTN_BW_MASK+1;
    
  u8g2_uint_t border_width = flags & U8G2_BTN_BW_MASK;

  int8_t a = u8g2_GetAscent(u8g2);
  int8_t d = u8g2_GetDescent(u8g2);
  
  uint8_t color_backup = u8g2->draw_color;
  
  
  if ( flags & U8G2_BTN_XFRAME )
  {
    border_width++;
    gap_frame = border_width;
    border_width++;
  }
  

  
  for(;;)
  {

    xx = x;
    xx -= padding_h;
    xx -= border_width;
    ww = w+2*padding_h+2*border_width;
    
    yy = y;
    yy += u8g2->font_calc_vref(u8g2);
    yy -= a;
    yy -= padding_v;
    yy -= border_width;
    hh = a-d+2*padding_v+2*border_width;
    if ( border_width == 0 )
      break;
    if ( border_width == gap_frame )
    {
      u8g2_SetDrawColor(u8g2, color_backup == 0 ? 1 : 0);
    }
    u8g2_DrawFrame(u8g2, xx, yy, ww, hh);
    u8g2_SetDrawColor(u8g2, color_backup);
    
    if ( flags & U8G2_BTN_SHADOW_MASK )
    {
      if ( border_width == (flags & U8G2_BTN_BW_MASK) )
      {
        u8g2_uint_t i;
        u8g2_uint_t shadow_gap = (flags & U8G2_BTN_SHADOW_MASK) >> U8G2_BTN_SHADOW_POS;
        shadow_gap--;
        for( i = 0; i < border_width; i++ )
        {
          u8g2_DrawHLine(u8g2, xx+border_width+shadow_gap,yy+hh+i+shadow_gap,ww);
          u8g2_DrawVLine(u8g2, xx+ww+i+shadow_gap,yy+border_width+shadow_gap,hh);
        }
      }
    }
    border_width--;
  } /* for */
  
  if ( flags & U8G2_BTN_INV )
  {
    u8g2_SetDrawColor(u8g2, 2);         /* XOR */
    u8g2_DrawBox(u8g2, xx, yy, ww, hh);
    u8g2_SetDrawColor(u8g2, color_backup);
  }
}

/**
 * @brief 绘制带UTF8文本的按钮
 * @param u8g2        u8g2显示结构体指针
 * @param x           文本位置X坐标
 * @param y           文本位置Y坐标
 * @param flags       按钮标志位
 * @param width       按钮最小宽度（像素），0表示使用文本实际宽度
 * @param padding_h   水平内边距（像素）
 * @param padding_v   垂直内边距（像素）
 * @param text        UTF8编码的文本字符串
 *
 * 此函数是按钮绘制的主要接口，会自动计算文本宽度，
 * 设置透明字体模式，绘制文本和按钮边框。
 */
void u8g2_DrawButtonUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t flags, u8g2_uint_t width, u8g2_uint_t padding_h, u8g2_uint_t padding_v, const char *text)
{
  u8g2_uint_t w = u8g2_GetUTF8Width(u8g2, text);  // 获取文本宽度

  u8g2_uint_t text_x_offset = 0;   // 文本X偏移（用于居中）

  if ( flags & U8G2_BTN_HCENTER )
    x -= (w+1)/2;    // 水平居中：调整X坐标

  if ( w < width )
  {
    if ( flags & U8G2_BTN_HCENTER )
    {
      text_x_offset = (width-w)/2;   // 计算文本在按钮内的偏移
    }
    w = width;   // 使用指定的最小宽度
  }

  u8g2_SetFontMode(u8g2, 1);           // 设置字体为透明模式
  u8g2_DrawUTF8(u8g2, x,y, text);      // 绘制文本
  u8g2_DrawButtonFrame(u8g2, x-text_x_offset, y, flags, w, padding_h, padding_v);  // 绘制边框

}



#ifdef NOT_USED
void u8g2_Draw4Pixel(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w)
{
  u8g2_DrawPixel(u8g2, x,y-1);
  u8g2_DrawPixel(u8g2, x+w-1,y-1);
  u8g2_DrawPixel(u8g2, x+w-1,y-w);
  u8g2_DrawPixel(u8g2, x,y-w);
}

void u8g2_DrawRadio(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t is_checked)
{
  uint8_t color_backup = u8g2->draw_color;
  u8g2_DrawCheckbox(u8g2, x,y,w,is_checked);
  u8g2_SetDrawColor(u8g2, 2);
  u8g2_Draw4Pixel(u8g2, x,y,w);
  if ( is_checked )
  {
    //u8g2_Draw4Pixel(u8g2, x+2,y-2,w-4);
  }
  
  u8g2_SetDrawColor(u8g2, color_backup );
}
#endif
 

#ifdef _THIS_CODE_SHOULD_BE_REWRITTEN_WITHOUT_PADWIDTH_

/*
  Shadow is not supported
  Note: radius must be at least as high as the border width

  border width | good radius values
  1             | 3, 5, 7, 8, ...
  2             | 3, 5, 7, 8, ...
  
*/

void u8g2_DrawRButtonUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t flags, u8g2_uint_t padding_h_or_width, u8g2_uint_t padding_v, u8g2_uint_t r, const char *text)
{
  u8g2_uint_t w = u8g2_GetUTF8Width(u8g2, text);
  //u8g2_uint_t w = u8g2_GetExactStrWidth(u8g2, text);
  
  u8g2_uint_t xx, yy, ww, hh;
  
  u8g2_uint_t border_width = flags & U8G2_BTN_BW_MASK;
  u8g2_uint_t padding_h = padding_h_or_width;
  u8g2_uint_t text_x_offset = 0;        // used for U8G2_BTN_PADWIDTH mode

  int8_t a = u8g2_GetAscent(u8g2);
  int8_t d = u8g2_GetDescent(u8g2);
  uint8_t color_backup = u8g2->draw_color;


  if ( flags & U8G2_BTN_HCENTER )
    x -= w/2;

  if ( flags & U8G2_BTN_PADWIDTH )
  {
    padding_h = 0;
    if ( w < padding_h_or_width )
    {
      if ( flags & U8G2_BTN_HCENTER )
      {
        text_x_offset = (padding_h_or_width-w)/2;
      }
      w = padding_h_or_width;
    }
  }
  
  
  u8g2_SetFontMode(u8g2, 1);
    
  for(;;)
  {
    if ( padding_h >= u8g2_GetDisplayWidth(u8g2)/2 )    // padding_h is zero if U8G2_BTN_PADWIDTH is set
    {
      xx = (flags & U8G2_BTN_BW_MASK) - border_width;
      ww = u8g2_GetDisplayWidth(u8g2);
      ww -= 2*((flags & U8G2_BTN_BW_MASK) - border_width);
      //printf("xx=%d ww=%d\n", xx, ww);
      //printf("clip_x1=%d clip_x0=%d\n", u8g2->clip_x1, u8g2->clip_x0);
    }
    else
    {
      xx = x;
      xx -= text_x_offset;
      xx -= padding_h;
      xx -= border_width;
      ww = w+2*padding_h+2*border_width;
    }
    
    yy = y;
    yy += u8g2->font_calc_vref(u8g2);
    yy -= a;
    yy -= padding_v;
    yy -= border_width;
    hh = a-d+2*padding_v+2*border_width;
    if ( border_width == 0 )
      break;
    u8g2_DrawRFrame(u8g2, xx, yy, ww, hh, r);
    if ( (flags & U8G2_BTN_BW_MASK) > 1 )
      u8g2_DrawRFrame(u8g2, xx, yy, ww, hh, r+1);
    
    border_width--;
    if ( r > 1 )
      r--;
  }
  if ( flags & U8G2_BTN_INV )
  {
    u8g2_DrawRBox(u8g2, xx, yy, ww, hh,r);
    u8g2_SetDrawColor(u8g2, 1-u8g2->draw_color);
  }
  u8g2_DrawUTF8(u8g2, x,y, text);
  u8g2_SetDrawColor(u8g2, color_backup);
}

#endif
