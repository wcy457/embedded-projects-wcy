/**
 * @file u8x8_setup.c
 * @brief u8x8初始化和设置函数实现
 *
 * 本文件提供了u8x8结构体的初始化和配置功能，包括：
 * - 默认回调函数（dummy callback）
 * - 空显示设备（null device）实现
 * - u8x8结构体的默认初始化
 * - 完整的u8x8设置流程
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
 * @brief 通用虚拟回调函数
 *
 * 作为所有回调函数的默认值。不处理任何消息，始终返回0（失败）。
 * 在u8x8_SetupDefaults中被设置为所有回调的默认值。
 *
 * @param u8x8 指向u8x8显示结构体的指针（未使用）
 * @param msg 消息类型（未使用）
 * @param arg_int 整数参数（未使用）
 * @param arg_ptr 指针参数（未使用）
 * @return 始终返回0（失败）
 */
uint8_t u8x8_dummy_cb(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
  return 0;    /* 不处理任何消息，返回失败 */
}


static const u8x8_display_info_t u8x8_null_display_info =
{
  /* chip_enable_level = */ 0,
  /* chip_disable_level = */ 1,
  
  /* post_chip_enable_wait_ns = */ 0,
  /* pre_chip_disable_wait_ns = */ 0,
  /* reset_pulse_width_ms = */ 0, 
  /* post_reset_wait_ms = */ 0, 
  /* sda_setup_time_ns = */ 0,		
  /* sck_pulse_width_ns = */ 0,	/* half of cycle time (100ns according to datasheet), AVR: below 70: 8 MHz, >= 70 --> 4MHz clock */
  /* sck_clock_hz = */ 4000000UL,	/* since Arduino 1.6.0, the SPI bus speed in Hz. Should be  1000000000/sck_pulse_width_ns */
  /* spi_mode = */ 0,		/* active high, rising edge */
  /* i2c_bus_clock_100kHz = */ 4,
  /* data_setup_time_ns = */ 0,
  /* write_pulse_width_ns = */ 0,
  /* tile_width = */ 1,		/* 8x8 */
  /* tile_hight = */ 1,
  /* default_x_offset = */ 0,
  /* flipmode_x_offset = */ 0,
  /* pixel_width = */ 8,
  /* pixel_height = */ 8
};


/**
 * @brief 空显示设备回调函数
 *
 * 用于测试或不需要实际控制显示的场景。
 * 处理SETUP_MEMORY和INIT消息，其他消息直接返回成功。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（未使用）
 * @param arg_ptr 指针参数（未使用）
 * @return 始终返回1（成功）
 */
uint8_t u8x8_d_null_cb(u8x8_t *u8x8, uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_null_display_info);  /* 设置内存信息 */
      break;
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);     /* 执行初始化 */
      break;
  }
  return 1;    /* 空设备始终返回成功 */
}


/**
 * @brief 设置u8x8结构体的默认值
 *
 * 将u8x8结构体的所有字段初始化为默认值。
 * 所有回调函数设置为dummy_cb，引脚设置为U8X8_PIN_NONE。
 *
 * @param u8x8 指向u8x8结构体的指针
 */
void u8x8_SetupDefaults(u8x8_t *u8x8)
{
    u8x8->display_info = NULL;                          /* 显示信息指针为空 */
    u8x8->display_cb = u8x8_dummy_cb;                   /* 显示回调 = 虚拟回调 */
    u8x8->cad_cb = u8x8_dummy_cb;                       /* CAD回调 = 虚拟回调 */
    u8x8->byte_cb = u8x8_dummy_cb;                      /* 字节回调 = 虚拟回调 */
    u8x8->gpio_and_delay_cb = u8x8_dummy_cb;            /* GPIO回调 = 虚拟回调 */
    u8x8->is_font_inverse_mode = 0;                     /* 字体正常模式（非反色） */
    u8x8->utf8_state = 0;                               /* UTF8解码状态重置 */
    u8x8->bus_clock = 0;                                /* 总线时钟（默认不设置） */
    u8x8->i2c_address = 255;                            /* I2C地址（255=未设置） */
    u8x8->debounce_default_pin_state = 255;             /* 默认引脚状态（假设低电平有效） */

#ifdef U8X8_USE_PINS
  {
    uint8_t i;
    for( i = 0; i < U8X8_PIN_CNT; i++ )
      u8x8->pins[i] = U8X8_PIN_NONE;                   /* 所有引脚设置为未使用 */
  }
#endif
}


/**
 * @brief 完整设置u8x8结构体
 *
 * 初始化u8x8结构体并分配回调函数。
 * 此函数不会与显示设备通信，只完成软件层面的初始化。
 * 调用u8x8_InitDisplay()来发送启动代码到显示设备。
 *
 * @param u8x8 指向u8x8结构体的指针
 * @param display_cb 显示控制器特定的回调函数（处理DRAW_TILE、INIT等消息）
 * @param cad_cb CAD通信层回调函数（处理命令/参数/数据的发送）
 * @param byte_cb 字节层回调函数（处理底层字节传输：SPI/I2C/并行）
 * @param gpio_and_delay_cb GPIO和延时回调函数（处理引脚操作和延时）
 */
void u8x8_Setup(u8x8_t *u8x8, u8x8_msg_cb display_cb, u8x8_msg_cb cad_cb, u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb)
{
  u8x8_SetupDefaults(u8x8);       /* 设置默认值，重置引脚为U8X8_PIN_NONE */

  /* 分配特定的回调函数 */
  u8x8->display_cb = display_cb;
  u8x8->cad_cb = cad_cb;
  u8x8->byte_cb = byte_cb;
  u8x8->gpio_and_delay_cb = gpio_and_delay_cb;

  u8x8_SetupMemory(u8x8);         /* 初始化显示内存信息 */
}

