/*

  u8x8_input_value.c
  
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
 * 文件: u8x8_input_value.c
 * 功能: u8x8库的数值输入界面组件
 *
 * 本文件实现了一个用户友好的数值输入界面，用于在OLED显示屏上
 * 通过上下按键选择0~255范围内的数值。
 *
 * 典型应用场景:
 *   - 调节显示屏亮度/对比度
 *   - 设置时间参数(小时、分钟)
 *   - 调整传感器阈值
 *   - 配置系统参数
 *
 * 在STM32智能手表项目中，此组件可用于设置界面中的各种数值调节。
 */

#include "u8x8.h"

/*
 * 函数: u8x8_UserInterfaceInputValue
 * 功能: 在OLED显示屏上显示数值输入界面，用户可通过按键选择数值
 *
 * 界面布局:
 *   ┌─────────────────────┐
 *   │       标题          │  <- title(居中显示)
 *   │    前缀 数值 后缀    │  <- pre + value + post(居中显示，当前值高亮)
 *   └─────────────────────┘
 *
 * 参数:
 *   u8x8   - u8x8显示结构体指针
 *   title  - 界面标题字符串(支持多行，用'\n'分隔)
 *   pre    - 数值前缀字符串(如"Value: ")
 *   value  - 指向当前数值的指针(输入/输出参数，用户确认后更新)
 *   lo     - 允许的最小值(0~255)
 *   hi     - 允许的最大值(0~255)
 *   digits - 数值显示的位数(如2表示显示"00"~"99")
 *   post   - 数值后缀字符串(如" %")
 *
 * 返回值:
 *   0 - 用户按下HOME键取消输入，数值未改变
 *   1 - 用户按下SELECT键确认输入，数值已更新
 */
uint8_t u8x8_UserInterfaceInputValue(u8x8_t *u8x8, const char *title, const char *pre, uint8_t *value, uint8_t lo, uint8_t hi, uint8_t digits, const char *post)
{
  uint8_t height;          /* 输入框总高度(行数) */
  uint8_t y;               /* 当前绘制的Y坐标(行号) */
  uint8_t width;           /* 数值区域总宽度(列数) */
  uint8_t x;               /* 当前绘制的X坐标(列号) */
  uint8_t local_value = *value;  /* 本地数值副本，确认后才写回原值 */
  uint8_t r;               /* 返回值 */
  uint8_t event;           /* 按键事件 */

  /* ========== 计算输入框布局 ========== */

  /* 计算输入框总高度: 标题行数 + 1行数值输入区 */
  height = 1;	/* 数值输入行(包含前缀、数值、后缀) */
  height += u8x8_GetStringLineCnt(title);  /* 加上标题行数 */

  /* 计算垂直居中偏移量 */
  y = 0;
  if ( height < u8x8_GetRows(u8x8)  )  /* 如果显示区域有剩余空间 */
  {
    y = u8x8_GetRows(u8x8);   /* 获取屏幕总行数 */
    y -= height;               /* 减去内容高度 */
    y /= 2;                   /* 除以2得到上边距 */
  }

  /* 计算水平居中偏移量 */
  x = 0;
  width = u8x8_GetUTF8Len(u8x8, pre);   /* 前缀宽度(UTF8字符数) */
  width += digits;                        /* 加上数值位数 */
  width += u8x8_GetUTF8Len(u8x8, post);  /* 加上后缀宽度 */
  if ( width < u8x8_GetCols(u8x8) )      /* 如果屏幕宽度有剩余 */
  {
    x = u8x8_GetCols(u8x8);   /* 获取屏幕总列数 */
    x -= width;                /* 减去内容宽度 */
    x /= 2;                   /* 除以2得到左边距 */
  }

  /* ========== 绘制界面 ========== */

  u8x8_ClearDisplay(u8x8);              /* 清屏(因为不是所有区域都会被填充) */
  u8x8_SetInverseFont(u8x8, 0);         /* 关闭反色显示 */
  y += u8x8_DrawUTF8Lines(u8x8, 0, y, u8x8_GetCols(u8x8), title);  /* 绘制标题并获取占用行数 */
  x += u8x8_DrawUTF8(u8x8, x, y, pre);                              /* 绘制前缀 */
  u8x8_DrawUTF8(u8x8, x+digits, y, post);                           /* 绘制后缀 */
  u8x8_SetInverseFont(u8x8, 1);         /* 开启反色显示(用于高亮当前数值) */

  /* ========== 按键事件循环 ========== */

  u8x8_DrawUTF8(u8x8, x, y, u8x8_u8toa(local_value, digits));  /* 初始显示当前数值 */
  for(;;)
  {
    event = u8x8_GetMenuEvent(u8x8);     /* 获取按键事件(阻塞等待) */
    if ( event == U8X8_MSG_GPIO_MENU_SELECT )    /* SELECT键: 确认输入 */
    {
      *value = local_value;    /* 将本地值写回输出参数 */
      r = 1;                   /* 返回1表示数值已更新 */
      break;
    }
    else if ( event == U8X8_MSG_GPIO_MENU_HOME )  /* HOME键: 取消输入 */
    {
      r = 0;                   /* 返回0表示取消 */
      break;
    }
    else if ( event == U8X8_MSG_GPIO_MENU_NEXT || event == U8X8_MSG_GPIO_MENU_UP )  /* NEXT/UP键: 数值+1 */
    {
      if ( local_value >= hi )   /* 如果已达到最大值 */
	local_value = lo;        /* 循环到最小值(溢出回绕) */
      else
	local_value++;           /* 数值加1 */
      u8x8_DrawUTF8(u8x8, x, y, u8x8_u8toa(local_value, digits));  /* 重新绘制数值 */
    }
    else if ( event == U8X8_MSG_GPIO_MENU_PREV || event == U8X8_MSG_GPIO_MENU_DOWN )  /* PREV/DOWN键: 数值-1 */
    {
      if ( local_value <= lo )   /* 如果已达到最小值 */
	local_value = hi;        /* 循环到最大值(下溢回绕) */
      else
	local_value--;           /* 数值减1 */
      u8x8_DrawUTF8(u8x8, x, y, u8x8_u8toa(local_value, digits));  /* 重新绘制数值 */
    }
  }

  u8x8_SetInverseFont(u8x8, 0);  /* 恢复正常显示模式 */
  return r;                       /* 返回结果(0=取消, 1=确认) */
}
