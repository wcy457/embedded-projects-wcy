/*

  u8g2_input_value.c
  
  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

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

/*
 * 【文件功能说明】
 * 本文件实现了u8g2库的数值输入对话框功能。
 * 提供一个图形界面，允许用户通过按键上下调整一个8位无符号整数值，
 * 数值在指定的最小值(lo)和最大值(hi)之间循环变化。
 * 适用于嵌入式设备上的参数设置、数值选择等交互场景（如STM32智能手表）。
 */

#include "u8g2.h"

/*
 * 【函数功能】用户界面数值输入对话框
 * 在屏幕上显示一个带标题的数值输入界面，用户可以通过上/下键（或前/后键）
 * 调整数值，按确认键保存，按HOME键取消。
 *
 * 【参数说明】
 *   u8g2   - u8g2显示结构体指针，包含显示驱动和字体信息
 *   title  - 对话框标题字符串，支持多行（用'\n'分隔）
 *   pre    - 数值前面的前缀文本（如"Value: "）
 *   value  - 指向当前值的指针，用户确认后会被更新为新值
 *   lo     - 数值的最小值（下限）
 *   hi     - 数值的最大值（上限）
 *   digits - 数值显示的位数（如3位则显示"000"~"255"）
 *   post   - 数值后面的后缀文本（如" bpm"）
 *
 * 【返回值】
 *   0 - 用户按HOME键取消，值未改变
 *   1 - 用户按确认键，值已更新
 */
uint8_t u8g2_UserInterfaceInputValue(u8g2_t *u8g2, const char *title, const char *pre, uint8_t *value, uint8_t lo, uint8_t hi, uint8_t digits, const char *post)
{
  /* --- 局部变量声明 --- */
  uint8_t line_height;       /* 单行文本的高度（像素） */
  uint8_t height;            /* 对话框总高度（以行为单位） */
  u8g2_uint_t pixel_height;  /* 对话框总高度（像素） */
  u8g2_uint_t  y, yy;        /* y轴偏移量，用于垂直居中布局 */
  u8g2_uint_t  pixel_width;  /* 输入行的总像素宽度 */
  u8g2_uint_t  x, xx;        /* x轴偏移量，用于水平居中布局 */

  uint8_t local_value = *value;  /* 本地副本，用户调整时先修改此值，确认后才写回 */
  //uint8_t r; /* not used ??? */
  uint8_t event;             /* 按键事件变量 */

  /* --- 初始化显示设置 --- */
  /* 强制使用水平方向绘制字符串（不支持旋转） */
  u8g2_SetFontDirection(u8g2, 0);

  /* 强制使用基线位置作为字体参考点 */
  u8g2_SetFontPosBaseline(u8g2);

  /* 计算单行文本高度 = 上行高度 - 下行高度（即字母从基线到顶部+底部的距离） */
  line_height = u8g2_GetAscent(u8g2);
  line_height -= u8g2_GetDescent(u8g2);
  
  
  /* --- 计算对话框整体布局 --- */
  /* 计算总行数：1行用于数值输入 + 标题行数 */
  height = 1;	/* 数值输入行固定占1行 */
  height += u8x8_GetStringLineCnt(title);  /* 加上标题的行数 */

  /* 将行数转换为像素高度 */
  pixel_height = height;
  pixel_height *= line_height;

  /* --- 计算垂直方向偏移量（使内容垂直居中） --- */
  y = 0;
  if ( pixel_height < u8g2_GetDisplayHeight(u8g2)  )
  {
    y = u8g2_GetDisplayHeight(u8g2);   /* 获取屏幕总高度 */
    y -= pixel_height;                 /* 减去内容高度 */
    y /= 2;                           /* 除以2得到上边距 */
  }

  /* --- 计算水平方向偏移量（使输入行水平居中） --- */
  x = 0;
  /* 计算输入行总像素宽度 = 前缀宽度 + 数字宽度 + 后缀宽度 */
  pixel_width = u8g2_GetUTF8Width(u8g2, pre);              /* 前缀文本宽度 */
  pixel_width += u8g2_GetUTF8Width(u8g2, "0") * digits;    /* 数字部分宽度（每位数字宽度相同） */
  pixel_width += u8g2_GetUTF8Width(u8g2, post);            /* 后缀文本宽度 */
  if ( pixel_width < u8g2_GetDisplayWidth(u8g2) )
  {
    x = u8g2_GetDisplayWidth(u8g2);   /* 获取屏幕总宽度 */
    x -= pixel_width;                 /* 减去内容宽度 */
    x /= 2;                          /* 除以2得到左边距 */
  }
  
  /* --- 主事件循环（无限循环，直到用户做出选择） --- */
  for(;;)
  {
    /* u8g2页面缓冲机制：FirstPage开始，NextPage翻页，直到所有页绘制完成 */
    u8g2_FirstPage(u8g2);
    do
    {
      /* === 渲染界面内容 === */
      yy = y;
      /* 先绘制标题（支持多行），返回值为标题实际占用的高度 */
      yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title);

      /* 绘制数值输入行：前缀 + 当前数值 + 后缀 */
      xx = x;
      xx += u8g2_DrawUTF8(u8g2, xx, yy, pre);                             /* 绘制前缀文本 */
      xx += u8g2_DrawUTF8(u8g2, xx, yy, u8x8_u8toa(local_value, digits)); /* 将数值转为字符串并绘制 */
      u8g2_DrawUTF8(u8g2, xx, yy, post);                                  /* 绘制后缀文本 */
    } while( u8g2_NextPage(u8g2) );

#ifdef U8G2_REF_MAN_PIC
      return 0;
#endif

    /* === 等待并处理用户按键事件 === */
    for(;;)
    {
      event = u8x8_GetMenuEvent(u8g2_GetU8x8(u8g2));  /* 获取按键事件 */
      if ( event == U8X8_MSG_GPIO_MENU_SELECT )       /* 确认键：保存值并返回 */
      {
	*value = local_value;   /* 将调整后的值写回原始变量 */
	return 1;              /* 返回1表示值已更新 */
      }
      else if ( event == U8X8_MSG_GPIO_MENU_HOME )    /* HOME键：取消操作 */
      {
	return 0;              /* 返回0表示值未改变 */
      }
      else if ( event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_UP )
      {
	/* 上/下一个键：数值递增，超过最大值则回绕到最小值 */
	if ( local_value >= hi )
	  local_value = lo;    /* 达到上限，回绕到下限 */
	else
	  local_value++;       /* 正常递增 */
	break;                 /* 跳出按键等待，重新渲染界面 */
      }
      else if ( event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_DOWN )
      {
	/* 上/前一个键：数值递减，低于最小值则回绕到最大值 */
	if ( local_value <= lo )
	  local_value = hi;    /* 达到下限，回绕到上限 */
	else
	  local_value--;       /* 正常递减 */
	break;                 /* 跳出按键等待，重新渲染界面 */
      }
    }
  }

  /* 代码永远不会执行到这里（上方的无限循环总会通过return退出） */
  //return r;
}
