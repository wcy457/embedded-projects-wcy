/*

  u8x8_message.c
  
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
 * 文件: u8x8_message.c
 * 功能: u8x8库的消息对话框组件
 *
 * 本文件实现了在OLED显示屏上显示消息对话框的功能，支持:
 *   - 多行标题文本显示
 *   - 多个可选择按钮
 *   - 按钮高亮和上下切换
 *   - 用户确认/取消操作
 *
 * 典型应用场景:
 *   - 显示提示信息("保存成功"、"操作完成")
 *   - 确认对话框("是否删除？ [是] [否]")
 *   - 多选项对话框("[确定] [取消] [重试]")
 *
 * 在STM32智能手表项目中，此组件用于各种用户交互确认场景。
 */

#include "u8x8.h"

/*
 * 函数: u8x8_draw_button_line
 * 功能: 在指定行绘制按钮组，当前选中的按钮以反色(高亮)显示
 *
 * 参数:
 *   u8x8   - u8x8显示结构体指针
 *   y      - 按钮行的Y坐标(行号)
 *   w      - 可用宽度(列数)，用于计算居中偏移
 *   cursor - 当前选中按钮的索引(从0开始)，该按钮将高亮显示
 *   s      - 按钮文本字符串，多个按钮用'\n'分隔，以'\0'结尾
 *            例如: "OK\nCancel\nRetry"
 *
 * 返回值: 按钮总数
 */
uint8_t u8x8_draw_button_line(u8x8_t *u8x8, uint8_t y, uint8_t w, uint8_t cursor, const char *s)
{
  uint8_t i;       /* 循环计数器 */
  uint8_t cnt;     /* 按钮总数 */
  uint8_t total;   /* 所有按钮文本的总宽度 */
  uint8_t d;       /* 左侧偏移量(用于居中) */
  uint8_t x;       /* 当前绘制的X坐标 */
  cnt = u8x8_GetStringLineCnt(s);  /* 获取按钮数量(按'\n'分隔的行数) */

  /* 计算所有按钮的总宽度 */
  total = 0;
  for( i = 0; i < cnt; i++ )
  {
    total += u8x8_GetUTF8Len(u8x8, u8x8_GetStringLineStart(i, s));  /* 累加每个按钮的文本宽度 */
  }
  total += (cnt-1);	/* 按钮之间各加1个空格的间隔 */

  /* 计算水平居中偏移量 */
  d = 0;
  if ( total < w )       /* 如果总宽度小于可用宽度 */
  {
    d = w;               /* 可用宽度 */
    d -= total;          /* 减去按钮总宽度 */
    d /= 2;             /* 除以2得到左边距 */
  }

  /* 绘制按钮 */
  x = d;                          /* 从偏移位置开始绘制 */
  u8x8_SetInverseFont(u8x8, 0);   /* 默认正常显示 */
  for( i = 0; i < cnt; i++ )
  {
    if ( i == cursor )              /* 如果是当前选中的按钮 */
      u8x8_SetInverseFont(u8x8, 1);  /* 开启反色(高亮) */

    x+=u8x8_DrawUTF8(u8x8, x, y, u8x8_GetStringLineStart(i, s));  /* 绘制按钮文本 */
    u8x8_SetInverseFont(u8x8, 0);  /* 恢复正常显示 */
    x+=u8x8_DrawUTF8(u8x8, x, y, " ");  /* 绘制按钮间空格 */
  }

  return cnt;   /* 返回按钮总数 */
}

/*
 * 函数: u8x8_UserInterfaceMessage
 * 功能: 在OLED显示屏上显示消息对话框，包含标题和可选择的按钮
 *
 * 界面布局:
 *   ┌──────────────────────┐
 *   │    title1 (多行)     │  <- 第一段标题
 *   │    title2 (单行)     │  <- 第二段标题(可选)
 *   │    title3 (多行)     │  <- 第三段标题
 *   │   [按钮1] [按钮2]   │  <- 底部按钮行
 *   └──────────────────────┘
 *
 * 参数:
 *   u8x8   - u8x8显示结构体指针
 *   title1 - 第一段标题，支持多行(用'\n'分隔)
 *   title2 - 第二段标题，单行文本(可为NULL)。可接受u8x8_GetStringLineStart()的返回值
 *   title3 - 第三段标题，支持多行(用'\n'分隔)
 *   buttons- 按钮文本，多个按钮用'\n'分隔，以'\0'结尾
 *            例如: "OK\nCancel"
 *
 * 返回值:
 *   0     - 用户按下HOME键取消(无选择)
 *   1~N   - 用户选择的按钮编号(从1开始，对应buttons中的顺序)
 */
uint8_t u8x8_UserInterfaceMessage(u8x8_t *u8x8, const char *title1, const char *title2, const char *title3, const char *buttons)
{
  uint8_t height;          /* 消息框总高度(行数) */
  uint8_t y;               /* 当前绘制的Y坐标 */
  uint8_t cursor = 0;      /* 当前选中按钮的索引(从0开始) */
  uint8_t button_cnt;      /* 按钮总数 */
  uint8_t event;           /* 按键事件 */

  u8x8_SetInverseFont(u8x8, 0);  /* 确保初始为正常显示模式 */

  /* ========== 计算消息框布局 ========== */

  /* 计算总高度: 按钮行(1行) + 各段标题行数 */
  height = 1;	/* 按钮行占1行 */
  height += u8x8_GetStringLineCnt(title1);     /* 第一段标题行数 */
  if ( title2 != NULL )
    height ++;                                  /* 第二段标题(单行) */
  height += u8x8_GetStringLineCnt(title3);     /* 第三段标题行数 */

  /* 计算垂直居中偏移量 */
  y = 0;
  if ( height < u8x8_GetRows(u8x8)  )         /* 如果屏幕有剩余空间 */
  {
    y = u8x8_GetRows(u8x8);                    /* 屏幕总行数 */
    y -= height;                                /* 减去内容高度 */
    y /= 2;                                    /* 上边距(垂直居中) */
  }

  /* ========== 绘制消息框 ========== */

  u8x8_ClearDisplay(u8x8);                     /* 清屏 */

  /* 绘制第一段标题(支持多行，返回实际占用行数) */
  y += u8x8_DrawUTF8Lines(u8x8, 0, y, u8x8_GetCols(u8x8), title1);
  if ( title2 != NULL )                         /* 如果有第二段标题 */
  {
    u8x8_DrawUTF8Line(u8x8, 0, y, u8x8_GetCols(u8x8), title2);  /* 绘制单行标题 */
    y++;                                        /* Y坐标下移一行 */
  }
  /* 绘制第三段标题(支持多行) */
  y += u8x8_DrawUTF8Lines(u8x8, 0, y, u8x8_GetCols(u8x8), title3);

  /* 绘制底部按钮行，第一个按钮默认高亮 */
  button_cnt = u8x8_draw_button_line(u8x8, y, u8x8_GetCols(u8x8), cursor, buttons);

  /* ========== 按键事件循环 ========== */

  for(;;)
  {
    event = u8x8_GetMenuEvent(u8x8);           /* 获取按键事件(阻塞等待) */
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )  /* SELECT键: 确认选择 */
      return cursor+1;                          /* 返回选中按钮编号(从1开始) */
    else if ( event == U8X8_MSG_GPIO_MENU_HOME )  /* HOME键: 取消 */
      break;                                    /* 跳出循环，返回0 */
    else if ( event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_UP )  /* NEXT/UP: 选择下一个按钮 */
    {
      cursor++;                                 /* 按钮索引+1 */
      if ( cursor >= button_cnt )               /* 如果超过最后一个 */
	cursor = 0;                             /* 循环到第一个按钮 */
      u8x8_draw_button_line(u8x8, y, u8x8_GetCols(u8x8), cursor, buttons);  /* 重绘按钮 */
    }
    else if ( event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_DOWN  )  /* PREV/DOWN: 选择上一个按钮 */
    {
      if ( cursor == 0 )                        /* 如果已在第一个按钮 */
	cursor = button_cnt;                    /* 循环到最后一个(后面会减1) */
      cursor--;                                 /* 按钮索引-1 */
      u8x8_draw_button_line(u8x8, y, u8x8_GetCols(u8x8), cursor, buttons);  /* 重绘按钮 */
    }
  }
  return 0;   /* 用户按下HOME键，返回0表示取消 */
}

