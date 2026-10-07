/**
 * @file u8x8_byte.c
 * @brief u8x8字节级通信接口实现
 *
 * 本文件实现了u8x8库与显示设备之间的字节级通信协议，包括：
 * - SPI通信协议（3线和4线软件SPI）
 * - I2C通信协议（软件I2C和硬件I2C）
 * - 并行通信协议（6800和8080模式）
 * - KS0108和SED1520等特殊显示控制器的通信接口
 *
 * 所有通信接口通过回调函数机制实现，用户可以根据实际硬件平台
 * 选择合适的通信方式。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 *
 * Copyright (c) 2016, olikraus@gmail.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or other
 *   materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "u8x8.h"
#include "i2c.h"

/**
 * @brief 设置数据/命令选择引脚（DC引脚）
 *
 * DC引脚用于区分发送的是命令还是数据：
 * - DC=0: 命令模式
 * - DC=1: 数据模式
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param dc DC引脚电平值（0或1）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_SetDC(u8x8_t *u8x8, uint8_t dc)
{
  return u8x8->byte_cb(u8x8, U8X8_MSG_BYTE_SET_DC, dc, NULL);
}

/**
 * @brief 发送多个字节数据
 *
 * 通过字节回调函数发送指定数量的字节数据到显示设备。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param cnt 要发送的字节数量
 * @param data 指向要发送的数据缓冲区
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_SendBytes(u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
{
  return u8x8->byte_cb(u8x8, U8X8_MSG_BYTE_SEND, cnt, (void *)data);
}

/**
 * @brief 发送单个字节数据
 *
 * 发送单个字节到显示设备，是u8x8_byte_SendBytes的便捷封装。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param byte 要发送的字节
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_SendByte(u8x8_t *u8x8, uint8_t byte)
{
  return u8x8_byte_SendBytes(u8x8, 1, &byte);
}

/**
 * @brief 开始数据传输
 *
 * 通知显示设备开始接收数据，通常用于拉低片选信号。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_StartTransfer(u8x8_t *u8x8)
{
  return u8x8->byte_cb(u8x8, U8X8_MSG_BYTE_START_TRANSFER, 0, NULL);
}

/**
 * @brief 结束数据传输
 *
 * 通知显示设备数据传输结束，通常用于释放片选信号。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_EndTransfer(u8x8_t *u8x8)
{
  return u8x8->byte_cb(u8x8, U8X8_MSG_BYTE_END_TRANSFER, 0, NULL);
}

/*=========================================*/

/**
 * @brief 空字节回调函数
 *
 * 不执行任何操作的字节回调函数，始终返回成功。
 * 用于不需要实际硬件通信的测试或模拟场景。
 *
 * @param u8x8 指向u8x8显示结构体的指针（未使用）
 * @param msg 消息类型
 * @param arg_int 整数参数（未使用）
 * @param arg_ptr 指针参数（未使用）
 * @return 始终返回1（成功）
 */
uint8_t u8x8_byte_empty(U8X8_UNUSED u8x8_t *u8x8, uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
    case U8X8_MSG_BYTE_INIT:
    case U8X8_MSG_BYTE_SET_DC:
    case U8X8_MSG_BYTE_START_TRANSFER:
    case U8X8_MSG_BYTE_END_TRANSFER:
      break;	/* 不执行任何操作 */
  }
  return 1;	/* 始终返回成功 */
}


/*=========================================*/

/**
 * @brief 4线软件SPI字节通信回调函数
 *
 * 实现标准的4线SPI通信协议（MOSI、MISO、SCK、CS）。
 * 使用GPIO模拟SPI时序，适用于没有硬件SPI外设或需要灵活配置的场景。
 *
 * 使用的display_info参数：
 *   - sda_setup_time_ns: 数据建立时间
 *   - sck_pulse_width_ns: 时钟脉冲宽度
 *   - spi_mode: SPI模式（决定时钟极性和相位）
 *   - chip_disable_level: 片选禁用电平
 *   - chip_enable_level: 片选使能电平
 *   - post_chip_enable_wait_ns: 片选使能后等待时间
 *   - pre_chip_disable_wait_ns: 片选禁用前等待时间
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数或DC值）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_4wire_sw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
  uint8_t takeover_edge = u8x8_GetSPIClockPhase(u8x8);      /* 获取数据采样边沿 */
  uint8_t not_takeover_edge = 1 - takeover_edge;             /* 数据切换边沿（与采样边沿相反） */

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;      /* 获取当前字节 */
	data++;         /* 移动到下一个字节 */
	arg_int--;      /* 减少剩余字节计数 */
	/* 逐位发送字节，MSB优先 */
	for( i = 0; i < 8; i++ )
	{
	  /* 设置数据线电平 */
	  if ( b & 128 )        /* 检查最高位 */
	    u8x8_gpio_SetSPIData(u8x8, 1);
	  else
	    u8x8_gpio_SetSPIData(u8x8, 0);
	  b <<= 1;              /* 左移准备下一位 */

	  /* 生成时钟信号 */
	  u8x8_gpio_SetSPIClock(u8x8, not_takeover_edge);        /* 时钟切换到非采样边沿 */
	  u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->sda_setup_time_ns);  /* 数据建立时间 */
	  u8x8_gpio_SetSPIClock(u8x8, takeover_edge);            /* 时钟切换到采样边沿 */
	  u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->sck_pulse_width_ns); /* 时钟脉冲宽度 */
	}
      }
      break;

    case U8X8_MSG_BYTE_INIT:
      /* 禁用片选 */
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* 设置SPI时钟初始电平 */
      u8x8_gpio_SetSPIClock(u8x8, u8x8_GetSPIClockPhase(u8x8));
      break;
    case U8X8_MSG_BYTE_SET_DC:
      u8x8_gpio_SetDC(u8x8, arg_int);    /* 设置DC引脚 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);    /* 使能片选 */
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);   /* 禁用片选 */
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

/**
 * @brief 8位6800模式并行通信回调函数
 *
 * 实现Motorola 6800系列并行总线协议，使用E（使能）信号控制数据传输。
 * 适用于6800系列兼容的LCD控制器。
 *
 * 时序：
 * 1. 设置8位数据到D0-D7
 * 2. 等待数据建立时间
 * 3. 拉高E信号（数据被锁存）
 * 4. 等待写脉冲宽度
 * 5. 拉低E信号
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数或DC值）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_8bit_6800mode(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;      /* 获取当前字节 */
	data++;         /* 移动到下一个字节 */
	arg_int--;      /* 减少剩余字节计数 */
	/* 将字节的8位分别设置到D0-D7数据线 */
	for( i = U8X8_MSG_GPIO_D0; i <= U8X8_MSG_GPIO_D7; i++ )
	{
	  u8x8_gpio_call(u8x8, i, b&1);    /* 设置最低位 */
	  b >>= 1;                          /* 右移准备下一位 */
	}

	/* 6800模式时序：数据建立 -> E脉冲 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);  /* 数据建立时间 */
	u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 1);    /* 拉高E信号，锁存数据 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns); /* E脉冲宽度 */
	u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 0);    /* 拉低E信号 */
      }
      break;

    case U8X8_MSG_BYTE_INIT:
      /* 禁用片选 */
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* 确保E信号初始为低 */
      u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 0);
      break;
    case U8X8_MSG_BYTE_SET_DC:
      u8x8_gpio_SetDC(u8x8, arg_int);    /* 设置DC引脚 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);    /* 使能片选 */
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);   /* 禁用片选 */
      break;
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief 8位8080模式并行通信回调函数
 *
 * 实现Intel 8080系列并行总线协议，使用WR（写使能）信号控制数据传输。
 * 适用于8080系列兼容的LCD控制器。
 *
 * 与6800模式的区别：
 * - 8080模式：WR信号下降沿锁存数据
 * - 6800模式：E信号高电平锁存数据
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数或DC值）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_8bit_8080mode(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;      /* 获取当前字节 */
	data++;         /* 移动到下一个字节 */
	arg_int--;      /* 减少剩余字节计数 */
	/* 将字节的8位分别设置到D0-D7数据线 */
	for( i = U8X8_MSG_GPIO_D0; i <= U8X8_MSG_GPIO_D7; i++ )
	{
	  u8x8_gpio_call(u8x8, i, b&1);    /* 设置最低位 */
	  b >>= 1;                          /* 右移准备下一位 */
	}

	/* 8080模式时序：数据建立 -> WR下降沿 -> WR上升沿 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);  /* 数据建立时间 */
	u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 0);    /* 拉低WR信号（下降沿锁存数据） */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns); /* WR脉冲宽度 */
	u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 1);    /* 拉高WR信号 */
      }
      break;

    case U8X8_MSG_BYTE_INIT:
      /* 禁用片选 */
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* 确保WR信号初始为高 */
      u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 1);
      break;
    case U8X8_MSG_BYTE_SET_DC:
      u8x8_gpio_SetDC(u8x8, arg_int);    /* 设置DC引脚 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);    /* 使能片选 */
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);   /* 禁用片选 */
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

/**
 * @brief 3线软件SPI字节通信回调函数
 *
 * 实现3线SPI通信协议（MOSI、SCK、CS），将DC信号嵌入数据流中。
 * 每个字节传输9位：第1位为DC标志，后8位为数据。
 * 适用于只有3根线连接的显示设备。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数或DC值）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_3wire_sw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i;
  uint8_t *data;
  uint8_t takeover_edge = u8x8_GetSPIClockPhase(u8x8);      /* 获取数据采样边沿 */
  uint8_t not_takeover_edge = 1 - takeover_edge;             /* 数据切换边沿 */
  uint16_t b;
  static uint8_t last_dc;    /* 保存上次的DC值，用于嵌入数据流 */

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;      /* 获取当前字节 */
	/* 将DC标志嵌入第9位（MSB） */
	if ( last_dc != 0 )
	  b |= 256;     /* 设置第9位为1（数据模式） */
	data++;         /* 移动到下一个字节 */
	arg_int--;      /* 减少剩余字节计数 */
	/* 发送9位数据（1位DC + 8位数据） */
	for( i = 0; i < 9; i++ )
	{
	  /* 设置数据线电平 */
	  if ( b & 256 )        /* 检查最高位（第9位） */
	    u8x8_gpio_SetSPIData(u8x8, 1);
	  else
	    u8x8_gpio_SetSPIData(u8x8, 0);
	  b <<= 1;              /* 左移准备下一位 */

	  /* 生成时钟信号 */
	  u8x8_gpio_SetSPIClock(u8x8, not_takeover_edge);        /* 时钟切换到非采样边沿 */
	  u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->sda_setup_time_ns);  /* 数据建立时间 */
	  u8x8_gpio_SetSPIClock(u8x8, takeover_edge);            /* 时钟切换到采样边沿 */
	  u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->sck_pulse_width_ns); /* 时钟脉冲宽度 */
	}
      }
      break;

    case U8X8_MSG_BYTE_INIT:
      /* 禁用片选 */
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* 设置SPI时钟初始电平 */
      u8x8_gpio_SetSPIClock(u8x8, u8x8_GetSPIClockPhase(u8x8));
      break;
    case U8X8_MSG_BYTE_SET_DC:
      last_dc = arg_int;    /* 保存DC值，用于后续数据发送 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);    /* 使能片选 */
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);   /* 禁用片选 */
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

/**
 * @brief 设置KS0108显示控制器的片选信号
 *
 * KS0108控制器使用3个片选信号（CS、CS1、CS2）来选择不同的显示区域。
 * 参数arg的位定义：
 *   - bit 0: CS信号
 *   - bit 1: CS1信号
 *   - bit 2: CS2信号
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param arg 片选信号值（3位）
 */
void u8x8_byte_set_ks0108_cs(u8x8_t *u8x8, uint8_t arg)
{
  u8x8_gpio_SetCS(u8x8, arg&1);    /* 设置CS信号 */
  arg = arg >> 1;
  u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_CS1, arg&1);    /* 设置CS1信号 */
  arg = arg >> 1;
  u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_CS2, arg&1);    /* 设置CS2信号 */
}

/**
 * @brief KS0108显示控制器字节通信回调函数
 *
 * 实现KS0108显示控制器的6800模式并行通信协议。
 * KS0108是一种常见的128x64图形LCD控制器。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
/* 6800模式 */
uint8_t u8x8_byte_ks0108(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;      /* 获取当前字节 */
	data++;         /* 移动到下一个字节 */
	arg_int--;      /* 减少剩余字节计数 */
	/* 将字节的8位分别设置到D0-D7数据线 */
	for( i = U8X8_MSG_GPIO_D0; i <= U8X8_MSG_GPIO_D7; i++ )
	{
	  u8x8_gpio_call(u8x8, i, b&1);    /* 设置最低位 */
	  b >>= 1;                          /* 右移准备下一位 */
	}

	/* 6800模式时序 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);  /* 数据建立时间 */
	u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 1);    /* 拉高E信号 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns); /* E脉冲宽度 */
	u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 0);    /* 拉低E信号 */
      }
      break;

    case U8X8_MSG_BYTE_INIT:
      /* 禁用片选 */
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* 确保E信号初始为低 */
      u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 0);
      break;
    case U8X8_MSG_BYTE_SET_DC:
      u8x8_gpio_SetDC(u8x8, arg_int);    /* 设置DC引脚 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      /* arg_int的低3位用于片选信号 */
      u8x8_byte_set_ks0108_cs(u8x8, arg_int);
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
      u8x8_byte_set_ks0108_cs(u8x8, arg_int);
      break;
    default:
      return 0;
  }
  return 1;
}


/**
 * @brief SED1520/SBN1661显示控制器字节通信回调函数
 *
 * 实现SED1520和SBN1661显示控制器的通信协议。
 * 这些控制器使用两个使能信号（E1和E2）来选择不同的显示区域：
 *   - U8X8_MSG_GPIO_E -> E1（左半屏）
 *   - U8X8_MSG_GPIO_CS -> E2（右半屏）
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（用于选择E1或E2）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_sed1520(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t i, b;
  uint8_t *data;
  static uint8_t enable_pin;    /* 当前使用的使能引脚（E1或E2） */

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
	b = *data;      /* 获取当前字节 */
	data++;         /* 移动到下一个字节 */
	arg_int--;      /* 减少剩余字节计数 */
	/* 将字节的8位分别设置到D0-D7数据线 */
	for( i = U8X8_MSG_GPIO_D0; i <= U8X8_MSG_GPIO_D7; i++ )
	{
	  u8x8_gpio_call(u8x8, i, b&1);    /* 设置最低位 */
	  b >>= 1;                          /* 右移准备下一位 */
	}

	/* 发送使能脉冲 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->data_setup_time_ns);  /* 数据建立时间 */
	u8x8_gpio_call(u8x8, enable_pin, 1);    /* 拉高使能信号 */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 200);    /* 额外延时（KS0108需要450ns） */
	u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->write_pulse_width_ns);  /* 写脉冲宽度 */
	u8x8_gpio_call(u8x8, enable_pin, 0);    /* 拉低使能信号 */
      }
      break;

    case U8X8_MSG_BYTE_INIT:
      /* 禁用片选 */
      u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
      /* 确保两个使能信号初始为低 */
      u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_E, 0);
      u8x8_gpio_call(u8x8, U8X8_MSG_GPIO_CS, 0);
      enable_pin = U8X8_MSG_GPIO_E;    /* 默认使用E1 */
      break;
    case U8X8_MSG_BYTE_SET_DC:
      u8x8_gpio_SetDC(u8x8, arg_int);    /* 设置DC引脚 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      /* SED1520/SBN1661不支持标准片选线 */
      /* 使用arg_int选择E1或E2使能信号 */
      enable_pin = U8X8_MSG_GPIO_E;      /* 默认使用E1（左半屏） */
      if ( arg_int != 0 )
	enable_pin = U8X8_MSG_GPIO_CS;    /* 使用E2（右半屏） */
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      break;
    default:
      return 0;
  }
  return 1;
}

/*=========================================*/

/**
 * @brief 软件I2C实现说明
 *
 * 本节实现了软件I2C（位敲击）通信协议。
 * 特点：
 * - 忽略ACK响应（某些显示设备不提供ACK）
 * - 不支持从设备读取数据
 * - 适用于没有硬件I2C外设的场景
 */

/**
 * @brief I2C时序延时函数
 *
 * 根据I2C总线时钟频率生成适当的延时。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_delay(u8x8_t *u8x8) U8X8_NOINLINE;
static void i2c_delay(u8x8_t *u8x8)
{
  u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_I2C, u8x8->display_info->i2c_bus_clock_100kHz);
}

/**
 * @brief 初始化I2C总线
 *
 * 将SCL和SDA线设置为高电平（空闲状态）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_init(u8x8_t *u8x8)
{
  u8x8_gpio_SetI2CClock(u8x8, 1);    /* SCL = 1 */
  u8x8_gpio_SetI2CData(u8x8, 1);    /* SDA = 1 */

  i2c_delay(u8x8);
}

/**
 * @brief 读取SCL线并延时
 *
 * 释放SCL线（设置为输入模式，由上拉电阻拉高），然后延时。
 * 注意：实际不读取SCL线状态，只是等待。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_read_scl_and_delay(u8x8_t *u8x8)
{
  /* 设置为输入模式（线路会被上拉电阻拉高） */
  u8x8_gpio_SetI2CClock(u8x8, 1);

  i2c_delay(u8x8);
}

/**
 * @brief 拉低SCL线
 *
 * 将SCL线驱动为低电平。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_clear_scl(u8x8_t *u8x8)
{
  u8x8_gpio_SetI2CClock(u8x8, 0);
}

/**
 * @brief 读取SDA线
 *
 * 释放SDA线（设置为输入模式，由上拉电阻拉高）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_read_sda(u8x8_t *u8x8)
{
  /* 设置为输入模式（线路会被上拉电阻拉高） */
  u8x8_gpio_SetI2CData(u8x8, 1);
}

/**
 * @brief 拉低SDA线
 *
 * 将SDA线驱动为低电平（开集电极输出）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_clear_sda(u8x8_t *u8x8)
{
  /* 设置为开集电极输出并驱动为低 */
  u8x8_gpio_SetI2CData(u8x8, 0);
}

/**
 * @brief 生成I2C起始条件
 *
 * 起始条件：SCL为高电平时，SDA从高变低。
 * 如果总线已经启动，则生成重复起始条件。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_start(u8x8_t *u8x8)
{
  if ( u8x8->i2c_started != 0 )
  {
    /* 如果已经启动，生成重复起始条件 */
    i2c_read_sda(u8x8);     /* SDA = 1 */
    i2c_delay(u8x8);
    i2c_read_scl_and_delay(u8x8);    /* SCL = 1 */
  }
  i2c_read_sda(u8x8);       /* SDA = 1 */
  /* 发送起始条件：SDA和SCL都从1变为0 */
  i2c_clear_sda(u8x8);      /* SDA = 0（SCL为高时SDA下降） */
  i2c_delay(u8x8);
  i2c_clear_scl(u8x8);      /* SCL = 0 */
  u8x8->i2c_started = 1;    /* 标记总线已启动 */
}

/**
 * @brief 生成I2C停止条件
 *
 * 停止条件：SCL为高电平时，SDA从低变高。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_stop(u8x8_t *u8x8)
{
  /* SDA = 0 */
  i2c_clear_sda(u8x8);
  i2c_delay(u8x8);

  /* 释放所有线路 */
  i2c_read_scl_and_delay(u8x8);    /* SCL = 1 */

  /* SDA = 1（SCL为高时SDA上升） */
  i2c_read_sda(u8x8);
  i2c_delay(u8x8);
  u8x8->i2c_started = 0;    /* 标记总线已停止 */
}

/**
 * @brief 写入一个I2C位
 *
 * 在SCL低电平时设置SDA，然后在SCL高电平时保持SDA稳定。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param val 要写入的位值（0或1）
 */
static void i2c_write_bit(u8x8_t *u8x8, uint8_t val)
{
  if (val)
    i2c_read_sda(u8x8);    /* SDA = 1 */
  else
    i2c_clear_sda(u8x8);   /* SDA = 0 */

  i2c_delay(u8x8);
  i2c_read_scl_and_delay(u8x8);    /* SCL = 1（数据被采样） */
  i2c_clear_scl(u8x8);             /* SCL = 0 */
}

/**
 * @brief 读取一个I2C位
 *
 * 释放SDA线，在SCL高电平时读取SDA状态。
 * 注意：当前实现不返回读取的值。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
static void i2c_read_bit(u8x8_t *u8x8)
{
  /* 不驱动SDA线 */
  i2c_read_sda(u8x8);              /* 释放SDA */
  i2c_delay(u8x8);
  i2c_read_scl_and_delay(u8x8);    /* SCL = 1 */
  i2c_read_sda(u8x8);              /* 读取SDA（实际未使用返回值） */
  i2c_delay(u8x8);
  i2c_clear_scl(u8x8);             /* SCL = 0 */
}

/**
 * @brief 写入一个I2C字节
 *
 * 逐位发送8位数据（MSB优先），然后读取ACK位。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param b 要发送的字节
 */
static void i2c_write_byte(u8x8_t *u8x8, uint8_t b)
{
  /* 从MSB到LSB逐位发送 */
  i2c_write_bit(u8x8, b & 128);    /* bit 7 */
  i2c_write_bit(u8x8, b & 64);     /* bit 6 */
  i2c_write_bit(u8x8, b & 32);     /* bit 5 */
  i2c_write_bit(u8x8, b & 16);     /* bit 4 */
  i2c_write_bit(u8x8, b & 8);      /* bit 3 */
  i2c_write_bit(u8x8, b & 4);      /* bit 2 */
  i2c_write_bit(u8x8, b & 2);      /* bit 1 */
  i2c_write_bit(u8x8, b & 1);      /* bit 0 */

  /* 读取从设备的ACK响应 */
  /* 0: 从设备确认 */
  /* 1: 无响应（某些设备可能不提供ACK） */
  i2c_read_bit(u8x8);
}

/**
 * @brief 硬件I2C字节通信回调函数
 *
 * 使用STM32 HAL库的硬件I2C接口进行通信。
 * 数据在START_TRANSFER和END_TRANSFER之间缓冲，然后一次性发送。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_hw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t buffer[32];    /* 数据缓冲区（u8g2/u8x8单次传输不超过32字节） */
  static uint8_t buf_idx;       /* 缓冲区索引 */
  uint8_t *data;

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      /* 将数据复制到缓冲区 */
      while( arg_int > 0 )
      {
	        buffer[buf_idx++] = *data;    /* 存入缓冲区 */
	        data++;                       /* 移动源指针 */
	        arg_int--;                    /* 减少剩余计数 */
      }
      break;
    case U8X8_MSG_BYTE_INIT:
      /* 在此处添加I2C外设初始化代码 */
      break;
    case U8X8_MSG_BYTE_SET_DC:
      /* I2C通信忽略DC信号 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      buf_idx = 0;    /* 重置缓冲区索引 */
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      /* 通过硬件I2C发送缓冲区中的数据 */
      HAL_I2C_Master_Transmit(&hi2c1, u8x8_GetI2CAddress(u8x8), buffer, buf_idx, 0x100);
      break;
    default:
      return 0;
  }
  return 1;
}


/*=========================================*/

/**
 * @brief 软件I2C字节通信回调函数（替代方案）
 *
 * 使用软件I2C（位敲击）实现的字节通信回调函数。
 * 需要定义ALTERNATIVE_I2C_BYTE_PROCEDURE宏才能启用。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
/* 替代I2C字节通信方案 */
#ifdef ALTERNATIVE_I2C_BYTE_PROCEDURE

/**
 * @brief I2C数据传输函数
 *
 * 执行完整的I2C传输序列：起始条件 -> 地址 -> 数据 -> 停止条件。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param adr I2C设备地址（7位）
 * @param cnt 要发送的数据字节数
 * @param data 指向要发送的数据缓冲区
 */
void i2c_transfer(u8x8_t *u8x8, uint8_t adr, uint8_t cnt, uint8_t *data)
{
  uint8_t i;
  i2c_start(u8x8);              /* 发送起始条件 */
  i2c_write_byte(u8x8, adr);   /* 发送设备地址 */
  for( i = 0; i < cnt; i++ )
    i2c_write_byte(u8x8, data[i]);    /* 发送数据字节 */
  i2c_stop(u8x8);               /* 发送停止条件 */
}

/**
 * @brief 软件I2C字节通信回调函数
 *
 * 使用软件I2C实现的字节通信回调函数。
 * 数据在START_TRANSFER和END_TRANSFER之间缓冲，然后通过软件I2C发送。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（发送字节数）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_byte_sw_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t buffer[32];    /* 数据缓冲区（u8g2/u8x8单次传输不超过32字节） */
  static uint8_t buf_idx;       /* 缓冲区索引 */
  uint8_t *data;

  switch(msg)
  {
    case U8X8_MSG_BYTE_SEND:
      data = (uint8_t *)arg_ptr;
      /* 将数据复制到缓冲区 */
      while( arg_int > 0 )
      {
	buffer[buf_idx++] = *data;    /* 存入缓冲区 */
	data++;                       /* 移动源指针 */
	arg_int--;                    /* 减少剩余计数 */
      }
      break;
    case U8X8_MSG_BYTE_INIT:
      i2c_init(u8x8);            /* 初始化I2C通信 */
      break;
    case U8X8_MSG_BYTE_SET_DC:
      /* I2C通信忽略DC信号 */
      break;
    case U8X8_MSG_BYTE_START_TRANSFER:
      buf_idx = 0;    /* 重置缓冲区索引 */
      break;
    case U8X8_MSG_BYTE_END_TRANSFER:
      /* 通过软件I2C发送缓冲区中的数据 */
      i2c_transfer(u8x8, u8x8_GetI2CAddress(u8x8), buf_idx, buffer);
      break;
    default:
      return 0;
  }
  return 1;
}


#endif
