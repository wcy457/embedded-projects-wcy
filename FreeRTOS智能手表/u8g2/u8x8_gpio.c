/**
 * @file u8x8_gpio.c
 * @brief u8x8 GPIO（通用输入输出）接口实现
 *
 * 本文件实现了u8x8库的GPIO抽象层。
 * 通过回调函数机制，将GPIO操作与具体硬件平台分离。
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
 * @brief GPIO回调函数调用封装
 *
 * 通过u8x8结构体中的gpio_and_delay_cb回调函数执行GPIO操作。
 * 这是一个统一的GPIO操作接口，将具体实现委托给用户提供的回调函数。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg GPIO消息类型（如U8X8_MSG_GPIO_SET_CS、U8X8_MSG_GPIO_SET_DC等）
 * @param arg GPIO参数（通常为引脚电平值：0或1）
 */
void u8x8_gpio_call(u8x8_t *u8x8, uint8_t msg, uint8_t arg)
{
  u8x8->gpio_and_delay_cb(u8x8, msg, arg, NULL);
}
