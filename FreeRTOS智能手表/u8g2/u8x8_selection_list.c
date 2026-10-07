/*

  u8x8_selection_list.c
  
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
 * 文件: u8x8_selection_list.c
 * 功能: u8x8库的可滚动选择列表组件
 *
 * 本文件实现了在OLED显示屏上显示可滚动选择列表的功能，支持:
 *   - 超出屏幕的长列表自动滚动
 *   - 当前选中项高亮显示
 *   - 上下键循环浏览
 *   - 可选标题行
 *
 * 典型应用场景:
 *   - 菜单选择(主菜单、子菜单)
 *   - 文件列表浏览
 *   - 设置选项列表
 *   - 任何需要从多个选项中选择一个的场景
 *
 * 在STM32智能手表项目中，此组件是主菜单和应用列表的核心UI组件。
 * 例如: 功能菜单("时钟\n心率\n步数\n设置\n关于")
 */

#include "u8x8.h"

/*
 * 函数: u8sl_Next
 * 功能: 将选择列表的光标移动到下一项
 *       如果已到最后一项，光标循环回第一项并重置滚动位置
 *       如果新位置超出可见区域，自动滚动列表
 *
 * 参数:
 *   u8sl - 选择列表状态结构体指针，包含:
 *          current_pos: 当前光标位置(从0开始)
 *          first_pos:   可见区域第一项的位置
 *          total:       列表总项数
 *          visible:     屏幕可见行数
 *
 * 返回值: 无
 */
void u8sl_Next(u8sl_t *u8sl)
{
  u8sl->current_pos++;                            /* 光标下移一项 */
  if ( u8sl->current_pos >= u8sl->total )         /* 如果超过最后一项 */
  {
    u8sl->current_pos = 0;                         /* 光标回到第一项 */
    u8sl->first_pos = 0;                           /* 滚动位置重置到开头 */
  }
  else
  {
    /* 检查新位置是否超出可见区域底部 */
    if ( u8sl->first_pos + u8sl->visible <= u8sl->current_pos + 1 )
    {
      /* 向下滚动列表，使新位置刚好在可见区域底部 */
      u8sl->first_pos = u8sl->current_pos - u8sl->visible + 1;
    }
  }
}

/*
 * 函数: u8sl_Prev
 * 功能: 将选择列表的光标移动到上一项
 *       如果已在第一项，光标循环到最后一项并自动调整滚动位置
 *       如果新位置在可见区域上方，自动向上滚动列表
 *
 * 参数:
 *   u8sl - 选择列表状态结构体指针
 *
 * 返回值: 无
 */
void u8sl_Prev(u8sl_t *u8sl)
{
  if ( u8sl->current_pos == 0 )                   /* 如果已在第一项 */
  {
    u8sl->current_pos = u8sl->total - 1;          /* 光标跳到最后一项 */
    u8sl->first_pos = 0;
    if ( u8sl->total > u8sl->visible )            /* 如果列表总项数超过可见行数 */
      u8sl->first_pos = u8sl->total - u8sl->visible;  /* 滚动到列表末尾 */
  }
  else
  {
    u8sl->current_pos--;                           /* 光标上移一项 */
    if ( u8sl->first_pos > u8sl->current_pos )    /* 如果新位置在可见区域上方 */
      u8sl->first_pos = u8sl->current_pos;        /* 向上滚动列表 */
  }
}

/*
 * 函数: u8x8_DrawSelectionList
 * 功能: 绘制选择列表的可见部分
 *       遍历可见区域的每一行，调用回调函数绘制每项内容
 *
 * 参数:
 *   u8x8  - u8x8显示结构体指针
 *   u8sl  - 选择列表状态结构体指针
 *   sl_cb - 绘制回调函数，负责绘制单行内容
 *   aux   - 传递给回调函数的辅助数据(通常是字符串列表)
 *
 * 返回值: 无
 */
void u8x8_DrawSelectionList(u8x8_t *u8x8, u8sl_t *u8sl, u8x8_sl_cb sl_cb, const void *aux)
{
  uint8_t i;
  for( i = 0; i < u8sl->visible; i++ )           /* 遍历所有可见行 */
  {
    sl_cb(u8x8, u8sl, i+u8sl->first_pos, aux);   /* 调用回调绘制每一行 */
  }
}

/*
 * 函数: u8x8_sl_string_line_cb
 * 功能: 选择列表的默认行绘制回调函数
 *       绘制单行文本，当前选中行以反色(高亮)显示
 *
 * 参数:
 *   u8x8 - u8x8显示结构体指针
 *   u8sl - 选择列表状态结构体指针
 *   idx  - 要绘制的项目索引(在整个列表中的位置)
 *   aux  - 辅助数据，指向以'\n'分隔的字符串列表
 *
 * 返回值: 无
 */
void u8x8_sl_string_line_cb(u8x8_t *u8x8, u8sl_t *u8sl, uint8_t idx, const void *aux)
{
  const char *s;     /* 指向当前行文本的指针 */
  uint8_t row;       /* 目标显示行号 */

  /* 计算目标行的Y坐标 */
  row = u8sl->y;             /* 从列表区域顶部开始 */
  row += idx;                /* 加上项目索引 */
  row -= u8sl->first_pos;   /* 减去第一项的偏移，得到屏幕上的实际行号 */

  /* 检查是否为当前选中行，设置高亮状态 */
  if ( idx == u8sl->current_pos )
    u8x8_SetInverseFont(u8x8, 1);   /* 选中行: 反色显示(白底黑字) */
  else
    u8x8_SetInverseFont(u8x8, 0);   /* 非选中行: 正常显示(黑底白字) */

  /* 从字符串列表中获取第idx行的文本 */
  s = u8x8_GetStringLineStart(idx, (const char *)aux);

  /* 绘制该行文本 */
  if ( s == NULL )           /* 如果该行为空 */
    s = "";                  /* 使用空字符串避免空指针 */
  u8x8_DrawUTF8Line(u8x8, u8sl->x, row, u8x8_GetCols(u8x8), s);  /* 绘制一行文本 */
  u8x8_SetInverseFont(u8x8, 0);   /* 恢复正常显示模式 */
}

/*
 * 函数: u8x8_UserInterfaceSelectionList
 * 功能: 在OLED显示屏上显示可滚动的选择列表，用户可通过按键浏览和选择
 *
 * 界面布局:
 *   ┌──────────────────────┐
 *   │    标题 (可选多行)    │  <- title(如果非NULL)
 *   │ > 选项1              │  <- 当前选中项(高亮)
 *   │   选项2              │
 *   │   选项3              │
 *   │   ...                │  <- 如果选项超出屏幕，支持滚动
 *   └──────────────────────┘
 *
 * 参数:
 *   u8x8     - u8x8显示结构体指针
 *   title    - 标题字符串(可为NULL表示无标题)，支持多行(用'\n'分隔)
 *   start_pos- 初始光标位置(从1开始计数)，0表示无默认选中
 *   sl       - 选项列表字符串，各选项用'\n'分隔，以'\0'结尾
 *              例如: "时钟\n心率\n步数\n设置\n关于"
 *
 * 返回值:
 *   0     - 用户按下HOME键取消选择
 *   1~N   - 用户选中的选项编号(从1开始，例如第一个选项返回1)
 */
uint8_t u8x8_UserInterfaceSelectionList(u8x8_t *u8x8, const char *title, uint8_t start_pos, const char *sl)
{
  u8sl_t u8sl;          /* 选择列表状态结构体 */
  uint8_t event;        /* 按键事件 */
  uint8_t title_lines;  /* 标题占用的行数 */

  if ( start_pos > 0 )
    start_pos--;         /* 将用户输入的1基索引转换为0基索引 */

  /* ========== 初始化列表状态 ========== */

  u8sl.visible = u8x8_GetRows(u8x8);     /* 可见行数 = 屏幕总行数 */
  u8sl.total = u8x8_GetStringLineCnt(sl); /* 总项数 = 字符串列表的行数 */
  u8sl.first_pos = 0;                     /* 初始滚动位置: 从第一项开始 */
  u8sl.current_pos = start_pos;           /* 初始光标位置 */
  u8sl.x = 0;                             /* 列表起始X坐标 */
  u8sl.y = 0;                             /* 列表起始Y坐标 */

  /* 绘制界面 */
  //u8x8_ClearDisplay(u8x8);             /* 不需要清屏，因为所有区域都会被填充 */
  u8x8_SetInverseFont(u8x8, 0);          /* 正常显示模式 */

  /* 如果有标题，绘制标题并调整列表区域 */
  if ( title != NULL )
  {
    title_lines = u8x8_DrawUTF8Lines(u8x8, u8sl.x, u8sl.y, u8x8_GetCols(u8x8), title);  /* 绘制标题 */
    u8sl.y+=title_lines;                  /* 列表起始Y坐标下移(标题下方) */
    u8sl.visible-=title_lines;            /* 减少可见行数(标题占用的部分) */
  }

  /* 边界检查: 确保初始光标不超过总项数 */
  if ( u8sl.current_pos >= u8sl.total )
    u8sl.current_pos = u8sl.total-1;      /* 限制到最后一项 */

  /* 绘制初始列表 */
  u8x8_DrawSelectionList(u8x8, &u8sl, u8x8_sl_string_line_cb, sl);

  /* ========== 按键事件循环 ========== */

  for(;;)
  {
    event = u8x8_GetMenuEvent(u8x8);       /* 获取按键事件(阻塞等待) */
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )    /* SELECT键: 确认选择 */
      return u8sl.current_pos+1;            /* 返回选中项编号(从1开始) */
    else if ( event == U8X8_MSG_GPIO_MENU_HOME )  /* HOME键: 取消选择 */
      return 0;                             /* 返回0表示取消 */
    else if ( event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_DOWN )  /* NEXT/DOWN: 下移光标 */
    {
      u8sl_Next(&u8sl);                     /* 移动光标到下一项(自动处理滚动) */
      u8x8_DrawSelectionList(u8x8, &u8sl, u8x8_sl_string_line_cb, sl);  /* 重绘列表 */
    }
    else if ( event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_UP  )  /* PREV/UP: 上移光标 */
    {
      u8sl_Prev(&u8sl);                     /* 移动光标到上一项(自动处理滚动) */
      u8x8_DrawSelectionList(u8x8, &u8sl, u8x8_sl_string_line_cb, sl);  /* 重绘列表 */
    }
  }
}

