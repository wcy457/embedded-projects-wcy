/*

  u8g2_message.c
  
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
 * 本文件实现了u8g2库的消息对话框功能。
 * 提供一个图形界面，可以显示多行文本消息和多个按钮供用户选择。
 * 按钮支持高亮选中和循环切换，适用于嵌入式设备上的确认对话框、
 * 选项选择等交互场景（如STM32智能手表的提示信息和选项确认）。
 */

#include "u8g2.h"

#define SPACE_BETWEEN_BUTTONS_IN_PIXEL 6           /* 按钮之间的水平间距（像素） */
#define SPACE_BETWEEN_TEXT_AND_BUTTONS_IN_PIXEL 3   /* 文本区域与按钮之间的垂直间距（像素） */

/*
 * 【函数功能】绘制按钮行
 * 在指定的y坐标处水平排列绘制多个按钮，当前选中的按钮以反色高亮显示。
 * 所有按钮会自动水平居中排列。
 *
 * 【参数说明】
 *   u8g2   - u8g2显示结构体指针
 *   y      - 按钮行的y坐标（基线位置）
 *   w      - 可用区域的总宽度（用于水平居中计算）
 *   cursor - 当前选中按钮的索引（从0开始），该按钮会被高亮显示
 *   s      - 按钮文本字符串，多个按钮用'\n'分隔（如"OK\nCancel"）
 *
 * 【返回值】按钮的数量
 */
uint8_t u8g2_draw_button_line(u8g2_t *u8g2, u8g2_uint_t y, u8g2_uint_t w, uint8_t cursor, const char *s)
{
  u8g2_uint_t button_line_width;  /* 所有按钮加间距的总宽度（像素） */

  uint8_t i;
  uint8_t cnt;           /* 按钮总数 */
  uint8_t is_invert;     /* 是否反色显示（1=选中高亮，0=正常显示） */

  u8g2_uint_t d;         /* 水平居中偏移量 */
  u8g2_uint_t x;         /* 当前绘制按钮的x坐标 */
	
  cnt = u8x8_GetStringLineCnt(s);
  
	
  /* --- 计算所有按钮的总宽度 --- */
  button_line_width = 0;
  for( i = 0; i < cnt; i++ )
  {
    /* 累加每个按钮文本的像素宽度 */
    button_line_width += u8g2_GetUTF8Width(u8g2, u8x8_GetStringLineStart(i, s));
  }
  /* 加上按钮之间的间距 */
  button_line_width += (cnt-1)*SPACE_BETWEEN_BUTTONS_IN_PIXEL;

  /* --- 计算水平居中偏移量 --- */
  d = 0;
  if ( button_line_width < w )
  {
    d = w;
    d -= button_line_width;  /* 计算剩余空间 */
    d /= 2;                 /* 左右平分得到左偏移 */
  }

  /* --- 逐个绘制按钮 --- */
  x = d;
  for( i = 0; i < cnt; i++ )
  {
    is_invert = 0;
    if ( i == cursor )
      is_invert = 1;         /* 当前光标位置的按钮使用反色高亮显示 */

    /* 绘制单个按钮（带1像素边框，选中则反色） */
    u8g2_DrawUTF8Line(u8g2, x, y, 0, u8x8_GetStringLineStart(i, s), 1, is_invert);
    x += u8g2_GetUTF8Width(u8g2, u8x8_GetStringLineStart(i, s));  /* 移动到下一个按钮位置 */
    x += SPACE_BETWEEN_BUTTONS_IN_PIXEL;                          /* 加上按钮间距 */
  }

  return cnt;  /* 返回按钮总数 */
}

/*
 * 【函数功能】用户界面消息对话框
 * 在屏幕上显示一个消息对话框，包含三段文本和多个按钮。
 * 用户可以通过按键在按钮之间切换，按确认键选择，按HOME键取消。
 *
 * 【参数说明】
 *   u8g2   - u8g2显示结构体指针
 *   title1 - 第一段标题文本，支持多行（用'\n'分隔）
 *   title2 - 第二段标题文本，单行字符串，以'\0'或'\n'结尾。
 *            可以传入u8x8_GetStringLineStart()的返回值，传NULL则不显示
 *   title3 - 第三段标题文本，支持多行（用'\n'分隔）
 *   buttons- 按钮文本，多个按钮用'\n'分隔，以'\0'结尾（如"OK\nCancel"）
 *
 * 【返回值】
 *   0     - 用户按HOME键取消
 *   1,2,3 - 用户选择的按钮编号（从1开始）
 *
 * 【副作用】会设置字体方向为水平(u8g2_SetFontDirection=0)，字体位置为基线(u8g2_SetFontPosBaseline)
 */
uint8_t u8g2_UserInterfaceMessage(u8g2_t *u8g2, const char *title1, const char *title2, const char *title3, const char *buttons)
{
  uint8_t height;        /* 消息框总高度（行数） */
  uint8_t line_height;   /* 单行文本的像素高度 */
  u8g2_uint_t pixel_height;  /* 消息框总像素高度 */
  u8g2_uint_t y, yy;     /* y轴偏移量，用于布局计算 */

  uint8_t cursor = 0;    /* 当前选中按钮的索引（从0开始） */
  uint8_t button_cnt;    /* 按钮总数 */
  uint8_t event;         /* 按键事件 */

  /* 强制水平方向绘制字符串 */
  u8g2_SetFontDirection(u8g2, 0);

  /* 强制基线位置作为字体参考点 */
  u8g2_SetFontPosBaseline(u8g2);


  /* 计算单行文本高度 = 上行高度 - 下行高度 */
  line_height = u8g2_GetAscent(u8g2);
  line_height -= u8g2_GetDescent(u8g2);

  /* 计算消息框总行数：按钮行 + 三段标题行 */
  height = 1;	/* 按钮行固定占1行 */
  height += u8x8_GetStringLineCnt(title1);   /* 第一段标题行数 */
  if ( title2 != NULL )
    height++;                                 /* 第二段标题占1行（如果非空） */
  height += u8x8_GetStringLineCnt(title3);   /* 第三段标题行数 */

  /* 将行数转换为像素高度 */
  pixel_height = height;
  pixel_height *= line_height;

  /* 加上文本与按钮之间的间距 */
  pixel_height +=SPACE_BETWEEN_TEXT_AND_BUTTONS_IN_PIXEL;

  /* 计算垂直方向偏移量（使消息框垂直居中） */
  y = 0;
  if ( pixel_height < u8g2_GetDisplayHeight(u8g2)   )
  {
    y = u8g2_GetDisplayHeight(u8g2);
    y -= pixel_height;  /* 剩余空间 */
    y /= 2;            /* 上边距 */
  }
  y += u8g2_GetAscent(u8g2);  /* 加上字体上升高度，定位到基线 */

  /* --- 主事件循环 --- */
  for(;;)
  {
      /* 开始页面绘制 */
      u8g2_FirstPage(u8g2);
      do
      {
	  yy = y;

	  /* === 绘制消息框内容 === */
	  /* 绘制第一段标题（支持多行） */
	  yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title1);
	  /* 绘制第二段标题（如果存在） */
	  if ( title2 != NULL )
	  {
	    u8g2_DrawUTF8Line(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), title2, 0, 0);
	    yy+=line_height;
	  }
	  /* 绘制第三段标题（支持多行） */
	  yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title3);
	  yy += SPACE_BETWEEN_TEXT_AND_BUTTONS_IN_PIXEL;  /* 文本与按钮之间的间距 */

	  /* 绘制按钮行，当前光标位置的按钮会被高亮 */
	  button_cnt = u8g2_draw_button_line(u8g2, yy, u8g2_GetDisplayWidth(u8g2), cursor, buttons);

      } while( u8g2_NextPage(u8g2) );  /* 继续下一页绘制（多缓冲区模式） */

#ifdef U8G2_REF_MAN_PIC
      return 0;
#endif

      /* === 等待并处理用户按键事件 === */
      for(;;)
      {
	    event = u8x8_GetMenuEvent(u8g2_GetU8x8(u8g2));  /* 获取按键事件 */
	    if ( event == U8X8_MSG_GPIO_MENU_SELECT )
	      return cursor+1;      /* 确认键：返回选中的按钮编号（从1开始） */
	    else if ( event == U8X8_MSG_GPIO_MENU_HOME )
	      return 0;             /* HOME键：返回0表示取消 */
	    else if ( event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_DOWN )
	    {
	      cursor++;             /* 移动到下一个按钮 */
	      if ( cursor >= button_cnt )
		cursor = 0;        /* 超出范围则回绕到第一个按钮 */
	      break;                /* 退出按键循环，重新渲染界面 */
	    }
	    else if ( event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_UP )
	    {
	      if ( cursor == 0 )
		cursor = button_cnt;  /* 在第一个按钮时上移，回绕到最后一个 */
	      cursor--;             /* 移动到上一个按钮 */
	      break;                /* 退出按键循环，重新渲染界面 */
	    }
      }
  }
  /* 代码永远不会执行到这里 */
  //return 0;
}

