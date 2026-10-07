/*

  u8g2_selection_list.c
  
  selection list with scroll option
  
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
 * 本文件实现了u8g2库的选择列表功能。
 * 提供可滚动的选择列表界面，用户可以通过上/下键在列表项之间导航，
 * 按确认键选择，按HOME键取消。支持标题显示、列表项高亮和滚动。
 * 适用于嵌入式设备上的菜单选择、设置列表等交互场景（如STM32智能手表）。
 */

#include "u8g2.h"

#define MY_BORDER_SIZE 1  /* 列表项选中时的边框宽度（像素） */

/*
 * 【函数功能】在指定位置绘制单行UTF-8文本
 * 将字符串在给定宽度内水平居中绘制，支持反色显示和边框。
 * 如果字符串宽度超出指定宽度，则左对齐显示。
 *
 * 【参数说明】
 *   u8g2        - u8g2显示结构体指针
 *   x           - 文本绘制起始x坐标
 *   y           - 文本绘制y坐标（基线位置）
 *   w           - 文本区域宽度（用于居中计算），0表示不居中
 *   s           - 要绘制的UTF-8字符串
 *   border_size - 边框厚度（像素），0表示无边框
 *   is_invert   - 是否反色显示（1=黑底白字，0=白底黑字）
 *
 * 【副作用】会设置字体方向为水平(u8g2_SetFontDirection=0)
 */
void u8g2_DrawUTF8Line(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, const char *s, uint8_t border_size, uint8_t is_invert)
{
  u8g2_uint_t d, str_width;  /* d=居中偏移量, str_width=字符串像素宽度 */
  u8g2_uint_t fx, fy, fw, fh;  /* 文本框的位置和尺寸 */

  /* 强制水平方向绘制字符串 */
  u8g2_SetFontDirection(u8g2, 0);

  /* 将y坐标转换为基线参考位置 */
  y += u8g2->font_calc_vref(u8g2);

  /* 计算字符串的像素宽度 */
  str_width = u8g2_GetUTF8Width(u8g2, s);

  /* 计算水平居中偏移量 */
  d = 0;
  if ( str_width < w )
  {
    d = w;
    d -=str_width;  /* 剩余空间 */
    d /= 2;        /* 左右平分 */
  }
  else
  {
    w = str_width;  /* 字符串超出宽度时，宽度等于字符串宽度 */
  }

  /* 计算文本框的边界坐标 */
  fx = x;
  fy = y - u8g2_GetAscent(u8g2) ;   /* 文本框顶部 = 基线 - 上行高度 */
  fw = w;
  fh = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2) ;  /* 文本框高度 = 上行 + 下行 */

  /* 如果是反色模式，先绘制填充矩形（白底变为黑底） */
  u8g2_SetDrawColor(u8g2, 1);
  if ( is_invert )
  {
    u8g2_DrawBox(u8g2, fx, fy, fw, fh);  /* 绘制实心矩形 */
  }

  /* 绘制边框（每增加1像素边框，矩形向外扩展1像素） */
  while( border_size > 0 )
  {
    fx--;       /* 左边界外扩 */
    fy--;       /* 上边界外扩 */
    fw +=2;     /* 宽度增加2（左右各1） */
    fh +=2;     /* 高度增加2（上下各1） */
    u8g2_DrawFrame(u8g2, fx, fy, fw, fh );  /* 绘制矩形边框 */
    border_size--;
  }

  /* 设置文字绘制颜色：反色模式用黑色字(0)，正常模式用白色字(1) */
  if ( is_invert )
  {
    u8g2_SetDrawColor(u8g2, 0);  /* 反色：黑字 */
  }
  else
  {
    u8g2_SetDrawColor(u8g2, 1);  /* 正常：白字 */
  }

  /* 绘制文本（x+d实现水平居中） */
  u8g2_DrawUTF8(u8g2, x+d, y, s);

  /* 恢复默认绘制颜色 */
  u8g2_SetDrawColor(u8g2, 1);

}


/*
 * 【函数功能】绘制多行UTF-8文本
 * 在指定位置绘制多行文本，每行文本可以相对于宽度w水平居中。
 * 多行文本以'\n'分隔存储在字符串s中。
 *
 * 【参数说明】
 *   u8g2        - u8g2显示结构体指针
 *   x           - 文本起始x坐标
 *   y           - 文本起始y坐标（基线位置）
 *   w           - 文本区域宽度（用于居中计算）
 *   line_height - 每行的像素高度（行间距）
 *   s           - 多行文本字符串，以'\n'分隔，以'\0'结尾
 *
 * 【返回值】所有行的总像素高度（行数 * line_height），如果s为NULL则返回0
 */
u8g2_uint_t u8g2_DrawUTF8Lines(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t w, u8g2_uint_t line_height, const char *s)
{
  uint8_t i;
  uint8_t cnt;           /* 文本总行数 */
  u8g2_uint_t yy = 0;    /* 累计绘制的高度 */
  cnt = u8x8_GetStringLineCnt(s);  /* 计算行数 */
  //printf("str=%s\n", s);
  //printf("cnt=%d, y=%d, line_height=%d\n", cnt, y, line_height);
  for( i = 0; i < cnt; i++ )
  {
    //printf("  i=%d, y=%d, line_height=%d\n", i, y, line_height);
    /* 绘制单行文本（无边框、无反色） */
    u8g2_DrawUTF8Line(u8g2, x, y, w, u8x8_GetStringLineStart(i, s), 0, 0);
    y+=line_height;   /* 移动到下一行位置 */
    yy+=line_height;  /* 累加高度 */
  }
  return yy;  /* 返回总绘制高度 */
}

/*
 * 【函数功能】绘制选择列表中的单个列表项
 * 根据索引从字符串列表中取出对应的列表项并绘制。
 * 如果该列表项是当前光标位置，则以反色高亮显示并带边框。
 *
 * 【参数说明】
 *   u8g2  - u8g2显示结构体指针
 *   u8sl  - 选择列表状态结构体（包含光标位置、可见项数量等）
 *   y     - 当前列表项的y坐标
 *   idx   - 列表项在字符串列表中的索引
 *   s     - 完整的列表字符串（多项以'\n'分隔）
 *
 * 【返回值】单行列表项的像素高度
 *
 * U8G2_NOINLINE属性：禁止内联，节省代码空间（嵌入式优化）
 */
static u8g2_uint_t u8g2_draw_selection_list_line(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, uint8_t idx, const char *s) U8G2_NOINLINE;
static u8g2_uint_t u8g2_draw_selection_list_line(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, uint8_t idx, const char *s)
{
  //u8g2_uint_t yy;
  uint8_t border_size = 0;  /* 边框宽度，默认为0（无边框） */
  uint8_t is_invert = 0;    /* 是否反色，默认为0（正常显示） */

  /* 计算行高 = 字体高度 + 边框大小 */
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2)+MY_BORDER_SIZE;

  /* calculate offset from display upper border */
  //yy = idx;
  //yy -= u8sl->first_pos;
  //yy *= line_height;
  //yy += y;

  /* 检查当前项是否为光标选中项 */
  if ( idx == u8sl->current_pos )
  {
    border_size = MY_BORDER_SIZE;  /* 选中项显示边框 */
    is_invert = 1;                 /* 选中项反色高亮 */
  }

  /* 从字符串列表中获取对应索引的文本 */
  s = u8x8_GetStringLineStart(idx, s);

  /* 绘制列表项文本（如果为NULL则显示空字符串） */
  if ( s == NULL )
    s = "";
  /* 绘制文本行：左右留MY_BORDER_SIZE像素边距 */
  u8g2_DrawUTF8Line(u8g2, MY_BORDER_SIZE, y, u8g2_GetDisplayWidth(u8g2)-2*MY_BORDER_SIZE, s, border_size, is_invert);
  return line_height;
}

/*
 * 【函数功能】绘制可见的选择列表项
 * 从第一个可见项开始，依次绘制所有可见的列表项。
 * 实现了列表的"窗口"效果——只显示可见范围内的列表项。
 *
 * 【参数说明】
 *   u8g2 - u8g2显示结构体指针
 *   u8sl - 选择列表状态结构体（包含first_pos、visible等信息）
 *   y    - 列表起始y坐标
 *   s    - 完整的列表字符串（多项以'\n'分隔）
 */
void u8g2_DrawSelectionList(u8g2_t *u8g2, u8sl_t *u8sl, u8g2_uint_t y, const char *s)
{
  uint8_t i;
  /* 遍历所有可见的列表项 */
  for( i = 0; i < u8sl->visible; i++ )
  {
    /* 绘制单个列表项，索引 = first_pos + i（实现滚动偏移） */
    y += u8g2_draw_selection_list_line(u8g2, u8sl, y, i+u8sl->first_pos, s);
  }
}


/*
 * 【函数功能】用户界面选择列表对话框
 * 在屏幕上显示一个可滚动的选择列表，支持标题、列表项导航和选择。
 * 用户可以通过上/下键滚动列表，按确认键选择当前项，按HOME键取消。
 * 列表项过多时会自动滚动，只显示可见范围内的列表项。
 *
 * 【参数说明】
 *   u8g2     - u8g2显示结构体指针
 *   title    - 标题字符串，支持多行（用'\n'分隔），传NULL则不显示标题
 *   start_pos- 光标的默认起始位置（从1开始计数），0表示不设默认位置
 *   sl       - 列表项字符串，多项以'\n'分隔，以'\0'结尾
 *
 * 【返回值】
 *   0        - 用户按HOME键取消
 *   1,2,3... - 用户选择的列表项编号（从1开始，与输入的列表顺序对应）
 *
 * 【副作用】会设置字体方向为水平(u8g2_SetFontDirection=0)，字体位置为基线(u8g2_SetFontPosBaseline)
 */
uint8_t u8g2_UserInterfaceSelectionList(u8g2_t *u8g2, const char *title, uint8_t start_pos, const char *sl)
{
  u8sl_t u8sl;           /* 选择列表状态结构体（滚动位置、光标位置等） */
  u8g2_uint_t yy;        /* y轴偏移量 */

  uint8_t event;         /* 按键事件 */

  /* 计算单行高度 = 字体高度 + 边框 */
  u8g2_uint_t line_height = u8g2_GetAscent(u8g2) - u8g2_GetDescent(u8g2)+MY_BORDER_SIZE;

  uint8_t title_lines = u8x8_GetStringLineCnt(title);  /* 标题行数 */
  uint8_t display_lines;  /* 屏幕可显示的总行数 */

  /* 将1基索引转换为0基索引（issue #112修正） */
  if ( start_pos > 0 )
    start_pos--;

  /* --- 计算可见列表项数量 --- */
  if ( title_lines > 0 )
  {
	/* 有标题时：总行数减去标题行数和3像素分隔线 */
	display_lines = (u8g2_GetDisplayHeight(u8g2)-3) / line_height;
	u8sl.visible = display_lines;
	u8sl.visible -= title_lines;  /* 减去标题占用的行数 */
  }
  else
  {
	/* 无标题时：全部用于显示列表项 */
	display_lines = u8g2_GetDisplayHeight(u8g2) / line_height;
	u8sl.visible = display_lines;
  }

  /* --- 初始化列表状态 --- */
  u8sl.total = u8x8_GetStringLineCnt(sl);  /* 列表项总数 */
  u8sl.first_pos = 0;                      /* 第一个可见项的索引 */
  u8sl.current_pos = start_pos;            /* 当前光标位置 */

  /* 确保光标位置在有效范围内 */
  if ( u8sl.current_pos >= u8sl.total )
    u8sl.current_pos = u8sl.total-1;
  /* 如果光标超出可见范围，调整滚动位置使光标可见 */
  if ( u8sl.first_pos+u8sl.visible <= u8sl.current_pos )
    u8sl.first_pos = u8sl.current_pos-u8sl.visible+1;

  /* 设置字体位置为基线 */
  u8g2_SetFontPosBaseline(u8g2);

  /* --- 主事件循环 --- */
  for(;;)
  {
      /* 开始页面绘制 */
      u8g2_FirstPage(u8g2);
      do
      {
        yy = u8g2_GetAscent(u8g2);  /* 初始y位置（基线偏移） */
        if ( title_lines > 0 )
        {
          /* 绘制标题文本 */
          yy += u8g2_DrawUTF8Lines(u8g2, 0, yy, u8g2_GetDisplayWidth(u8g2), line_height, title);

	  /* 在标题下方绘制水平分隔线 */
	  u8g2_DrawHLine(u8g2, 0, yy-line_height- u8g2_GetDescent(u8g2) + 1, u8g2_GetDisplayWidth(u8g2));

	  yy += 3;  /* 分隔线与列表之间的间距 */
        }
        /* 绘制选择列表（从当前滚动位置开始） */
        u8g2_DrawSelectionList(u8g2, &u8sl, yy, sl);
      } while( u8g2_NextPage(u8g2) );  /* 多缓冲区模式下继续下一页 */

#ifdef U8G2_REF_MAN_PIC
      return 0;
#endif

      /* === 等待并处理用户按键事件 === */
      for(;;)
      {
        event = u8x8_GetMenuEvent(u8g2_GetU8x8(u8g2));  /* 获取按键事件 */
        if ( event == U8X8_MSG_GPIO_MENU_SELECT )
          return u8sl.current_pos+1;    /* 确认键：返回选中项编号（+1转为1基索引） */
        else if ( event == U8X8_MSG_GPIO_MENU_HOME )
          return 0;                     /* HOME键：返回0表示取消 */
        else if ( event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_DOWN )
        {
          u8sl_Next(&u8sl);            /* 光标下移（内部处理滚动） */
          break;                        /* 退出按键循环，重新渲染界面 */
        }
        else if ( event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_UP )
        {
          u8sl_Prev(&u8sl);            /* 光标上移（内部处理滚动） */
          break;                        /* 退出按键循环，重新渲染界面 */
        }
      }
  }
}
