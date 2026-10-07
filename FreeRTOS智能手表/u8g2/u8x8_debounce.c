/**
 * @file u8x8_debounce.c
 * @brief 按键消抖算法实现（u8x8附加模块）
 *
 * 本文件实现了简单的按键消抖算法，用于处理机械按键的抖动问题。
 * 消抖状态机包含以下状态：
 *   状态A（空闲）: 等待按键按下
 *   状态B（消抖等待）: 等待消抖时间结束
 *   状态C（确认按下）: 按键已确认按下，等待释放
 *   状态D（等待释放）: 等待按键释放后触发事件
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

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

#include "u8x8.h"

/**
 * @brief 读取所有输入引脚状态
 *
 * 读取U8X8_PIN_INPUT_CNT个输入引脚的状态，将结果打包到一个字节中。
 * 每个引脚占1位，高电平为1，低电平为0。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @return 引脚状态字节（每位对应一个输入引脚）
 */
static uint8_t u8x8_read_pin_state(u8x8_t *u8x8)
{
  uint8_t i;
  uint8_t pin_state;

  pin_state = 255;	/* 初始值255，与默认引脚配置兼容（假设所有按键低电平有效） */
  for( i = 0; i < U8X8_PIN_INPUT_CNT; i++ )
  {
    pin_state <<= 1;    /* 左移，为下一位腾出空间 */

    /* 通过GPIO回调读取引脚状态，结果存入gpio_result */
    u8x8->gpio_result = 1;
    u8x8_gpio_call(u8x8, U8X8_MSG_GPIO(i+U8X8_PIN_OUTPUT_CNT), 0);
    pin_state |= u8x8->gpio_result & 1;  /* 将读取的位存入最低位 */
  }

  return pin_state;
}

/**
 * @brief 查找两个字节中第一个不同的位
 *
 * 从最低位开始比较，返回第一个不同位的位置。
 *
 * @param a 第一个字节
 * @param b 第二个字节
 * @return 不同位的位置（0到U8X8_PIN_INPUT_CNT-1），如果相同则返回U8X8_PIN_INPUT_CNT
 */
static uint8_t u8x8_find_first_diff(uint8_t a, uint8_t b)
{
  uint8_t mask;
  uint8_t i;
  mask = 1;                     /* 从最低位开始 */
  i = U8X8_PIN_INPUT_CNT;
  do
  {
    i--;
    if ( (a & mask) != (b & mask) )
      return i;                 /* 找到不同位，返回位置 */
    mask <<= 1;                 /* 移动到下一位 */
  } while( i > 0 );
  return U8X8_PIN_INPUT_CNT;    /* 没有不同位 */
}

/**
 * 消抖状态机说明：
 *
 * 状态A（空闲状态，0x00）:
 *   如果当前引脚状态与默认状态相同 -> 保持状态A
 *   如果不同 -> 进入状态B + 消抖计数
 *
 * 状态B + 计数（0x10 + cnt）:
 *   计数递减，等待消抖时间结束
 *
 * 状态B（0x10）:
 *   如果当前引脚状态恢复为默认 -> 回到状态A（抖动，忽略）
 *   如果仍然不同 -> 检测到有效按键，进入状态C
 *
 * 状态C（0x20）:
 *   如果当前引脚状态仍与上次相同 -> 进入状态D
 *   如果不同 -> 回到状态A（抖动，忽略）
 *
 * 状态D（0x30，等待释放）:
 *   如果当前引脚状态恢复为默认 -> 回到状态A，返回按键事件消息
 *   如果仍然按下 -> 保持状态D（可扩展自动重复功能）
 */

#ifdef __unix__xxxxxx_THIS_IS_DISABLED

#include <stdio.h>
#include <stdlib.h>
uint8_t u8x8_GetMenuEvent(u8x8_t *u8x8)
{
    int c;
    c = getc(stdin);
    switch(c)
    {
        case 'n':
            return  U8X8_MSG_GPIO_MENU_NEXT;
        case 'p':
            return  U8X8_MSG_GPIO_MENU_PREV;
        case 's':
            return  U8X8_MSG_GPIO_MENU_SELECT;
        case 'h':
            return  U8X8_MSG_GPIO_MENU_HOME;
        case 'x':
            exit(0);
        default:
            puts("press n, p, s, h or x");
            break;
    }
    return 0;
}


#else  /* __unix__ */


/** 消抖等待时间（状态计数） */
#define U8X8_DEBOUNCE_WAIT 2

/**
 * @brief 获取菜单事件（带消抖处理）
 *
 * 执行消抖算法并返回触发的GPIO消息。
 * 返回0表示没有事件发生。
 *
 * 消抖流程：
 * 1. 读取当前引脚状态
 * 2. 通过状态机判断是否有有效按键
 * 3. 在按键释放时返回对应的GPIO消息
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @return GPIO事件消息（如U8X8_MSG_GPIO_MENU_NEXT等），0表示无事件
 */
#if defined(__GNUC__) && !defined(__CYGWIN__)
# pragma weak  u8x8_GetMenuEvent    /* 弱符号，允许用户重写 */
#endif
uint8_t u8x8_GetMenuEvent(u8x8_t *u8x8)
{
  uint8_t pin_state;
  uint8_t result_msg = 0;	/* invalid message, no event */
  
  pin_state = u8x8_read_pin_state(u8x8);
  
  /* States A, B, C & D are encoded in the upper 4 bit*/
  switch(u8x8->debounce_state)
  {
    case 0x00:	/* State A, default state */
      if ( u8x8->debounce_default_pin_state != pin_state )
      {
	//u8x8->debounce_last_pin_state = pin_state;
	u8x8->debounce_state = 0x010 + U8X8_DEBOUNCE_WAIT;
      }
      break;
    case 0x10:	/* State B */
      //if ( u8x8->debounce_last_pin_state != pin_state )
      if ( u8x8->debounce_default_pin_state == pin_state )
      {
	u8x8->debounce_state = 0x00;	/* back to state A */
      }
      else
      {
	/* keypress detected */
	u8x8->debounce_last_pin_state = pin_state;
	//result_msg = U8X8_MSG_GPIO_MENU_NEXT;
	u8x8->debounce_state = 0x020 + U8X8_DEBOUNCE_WAIT;	/* got to state C */	
      }
      break;
      
    case 0x20:	/* State C */
      if ( u8x8->debounce_last_pin_state != pin_state )
      {
	u8x8->debounce_state = 0x00;	/* back to state A */
      }
      else
      {
	u8x8->debounce_state = 0x030;	/* got to state D */	
      }
      break;
      
    case 0x30:	/* State D */
      /* wait until key release */
      if ( u8x8->debounce_default_pin_state == pin_state )
      {
	u8x8->debounce_state = 0x00;	/* back to state A */
	result_msg = U8X8_MSG_GPIO(u8x8_find_first_diff(u8x8->debounce_default_pin_state, u8x8->debounce_last_pin_state)+U8X8_PIN_OUTPUT_CNT);
      }
      else
      {
	//result_msg = U8X8_MSG_GPIO_MENU_NEXT;
	// maybe implement autorepeat here 
      }
      break;
    default:
      u8x8->debounce_state--;	/* count down, until there is a valid state */
      break;
  }
  return result_msg;
}

#endif /* __unix__ */
