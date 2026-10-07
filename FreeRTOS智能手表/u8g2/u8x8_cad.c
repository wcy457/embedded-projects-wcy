/**
 * @file u8x8_cad.c
 * @brief u8x8命令/参数/数据(CAD)通信接口实现
 *
 * 本文件实现了u8x8库与显示控制器之间的CAD（Command-Arg-Data）抽象层。
 * CAD层位于字节通信层之上，负责将上层的命令、参数和数据转换为底层字节传输。
 *
 * 主要功能：
 * - 提供统一的命令/参数/数据发送接口
 * - 支持多种显示控制器的通信协议（SSD13xx、ST7920、UC16xx等）
 * - 实现I2C和SPI通信模式的CAD适配
 * - 提供格式化发送功能（u8x8_SendF）
 *
 * 使用流程：
 *   u8x8_cad_StartTransfer() -> 发送命令/参数/数据 -> u8x8_cad_EndTransfer()
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


  The following sequence must be used for any data, which is set to the display:
  
  
  uint8_t u8x8_cad_StartTransfer(u8x8_t *u8x8)

  any of the following calls
    uint8_t u8x8_cad_SendCmd(u8x8_t *u8x8, uint8_t cmd)
    uint8_t u8x8_cad_SendArg(u8x8_t *u8x8, uint8_t arg)
    uint8_t u8x8_cad_SendData(u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
  
  uint8_t u8x8_cad_EndTransfer(u8x8_t *u8x8)



*/
/*
uint8_t u8x8_cad_template(u8x8_t *u8x8, uint8_t msg, uint16_t arg_int, void *arg_ptr)
{
  uint8_t i;
  
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_mcd_byte_SetDC(mcd->next, 1);
      u8x8_mcd_byte_Send(mcd->next, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_mcd_byte_SetDC(mcd->next, 1);
      u8x8_mcd_byte_Send(mcd->next, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_mcd_byte_SetDC(mcd->next, 0);
      for( i = 0; i < 8; i++ )
	u8x8_mcd_byte_Send(mcd->next, ((uint8_t *)arg_ptr)[i]);
      break;
    case U8X8_MSG_CAD_RESET:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_END_TRANSFER:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    default:
      break;
  }
  return 1;
}

*/

#include "u8x8.h"

/**
 * @brief 发送命令到显示控制器
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param cmd 要发送的命令字节
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_SendCmd(u8x8_t *u8x8, uint8_t cmd)
{
  return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_CMD, cmd, NULL);
}

/**
 * @brief 发送命令参数到显示控制器
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param arg 要发送的参数字节
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_SendArg(u8x8_t *u8x8, uint8_t arg)
{
  return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_ARG, arg, NULL);
}

/**
 * @brief 发送多个相同的参数到显示控制器
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param cnt 要发送的参数数量
 * @param arg 要重复发送的参数值
 * @return 始终返回1（成功）
 */
uint8_t u8x8_cad_SendMultipleArg(u8x8_t *u8x8, uint8_t cnt, uint8_t arg)
{
  while( cnt > 0 )
  {
    u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_ARG, arg, NULL);
    cnt--;
  }
  return 1;
}

/**
 * @brief 发送数据到显示控制器
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param cnt 要发送的数据字节数
 * @param data 指向数据缓冲区的指针
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_SendData(u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
{
  return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, cnt, data);
}

/**
 * @brief 开始CAD数据传输
 *
 * 通常用于拉低片选信号，准备开始通信。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_StartTransfer(u8x8_t *u8x8)
{
  return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_START_TRANSFER, 0, NULL);
}

/**
 * @brief 结束CAD数据传输
 *
 * 通常用于释放片选信号，完成通信。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_EndTransfer(u8x8_t *u8x8)
{
  return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_END_TRANSFER, 0, NULL);
}

/**
 * @brief 格式化发送CAD数据（va_list版本）
 *
 * 根据格式字符串fmt中的字符类型，从可变参数列表中取值并发送：
 *   'a' - 发送参数（Argument）
 *   'c' - 发送命令（Command）
 *   'd' - 发送数据（Data）
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param fmt 格式字符串，每个字符决定下一个参数的发送类型
 * @param va 可变参数列表
 */
void u8x8_cad_vsendf(u8x8_t * u8x8, const char *fmt, va_list va)
{
  uint8_t d;
  u8x8_cad_StartTransfer(u8x8);    /* 开始传输 */
  while( *fmt != '\0' )
  {
    d = (uint8_t)va_arg(va, int);  /* 从可变参数中取出一个字节 */
    switch(*fmt)
    {
      case 'a':  u8x8_cad_SendArg(u8x8, d); break;     /* 发送参数 */
      case 'c':  u8x8_cad_SendCmd(u8x8, d); break;     /* 发送命令 */
      case 'd':  u8x8_cad_SendData(u8x8, 1, &d); break; /* 发送数据 */
    }
    fmt++;
  }
  u8x8_cad_EndTransfer(u8x8);      /* 结束传输 */
}

/**
 * @brief 格式化发送CAD数据（可变参数版本）
 *
 * 使用格式字符串和可变参数向显示控制器发送命令/参数/数据序列。
 * 格式字符串中的字符含义：
 *   'a' - 将对应参数作为Argument发送
 *   'c' - 将对应参数作为Command发送
 *   'd' - 将对应参数作为Data发送
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param fmt 格式字符串
 * @param ... 可变参数
 */
void u8x8_SendF(u8x8_t * u8x8, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);
  u8x8_cad_vsendf(u8x8, fmt, va);  /* 调用va_list版本 */
  va_end(va);
}

/**
 * @brief 发送预定义的CAD命令序列
 *
 * 数据序列格式：
 *   0x15 (21) + cmd  - 发送命令
 *   0x16 (22) + arg  - 发送参数
 *   0x17 (23) + data - 发送数据
 *   0x18 (24)        - 拉低CS（片选使能）
 *   0x19 (25)        - 拉高CS（片选禁用）
 *   0xFE (254) + ms  - 延时指定毫秒
 *   0xFF (255)       - 序列结束
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param data 指向命令序列数据的指针（以0xFF结尾）
 */
void u8x8_cad_SendSequence(u8x8_t *u8x8, uint8_t const *data)
{
  uint8_t cmd;
  uint8_t v;

  for(;;)
  {
    cmd = *data;        /* 读取命令字节 */
    data++;             /* 移动到下一个字节 */
    switch( cmd )
    {
      case U8X8_MSG_CAD_SEND_CMD:      /* 0x15: 发送命令 */
      case U8X8_MSG_CAD_SEND_ARG:      /* 0x16: 发送参数 */
	  v = *data;                      /* 读取参数值 */
	  u8x8->cad_cb(u8x8, cmd, v, NULL);
	  data++;
	  break;
      case U8X8_MSG_CAD_SEND_DATA:     /* 0x17: 发送数据 */
	  v = *data;
	  u8x8_cad_SendData(u8x8, 1, &v);
	  data++;
	  break;
      case U8X8_MSG_CAD_START_TRANSFER: /* 0x18: 开始传输（CS使能） */
      case U8X8_MSG_CAD_END_TRANSFER:   /* 0x19: 结束传输（CS禁用） */
	  u8x8->cad_cb(u8x8, cmd, 0, NULL);
	  break;
      case 0x0fe:                       /* 0xFE: 延时 */
	  v = *data;
	  u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, v);  /* 延时v毫秒 */
	  data++;
	  break;
      default:                          /* 0xFF或其他: 结束序列 */
	return;
    }
  }
}


/**
 * @brief 空CAD回调函数
 *
 * 最简单的CAD回调，直接将命令和参数作为单字节发送，不做DC信号控制。
 * 数据和传输控制直接传递给字节层回调。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数（命令/参数值或字节数）
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_empty(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SendByte(u8x8, arg_int);    /* 直接发送命令字节 */
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SendByte(u8x8, arg_int);    /* 直接发送参数字节 */
      break;
    case U8X8_MSG_CAD_SEND_DATA:
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);  /* 转发给字节层 */
    default:
      return 0;
  }
  return 1;
}


/**
 * @brief CAD 110模式回调函数
 *
 * DC信号分配规则（三位二进制表示：CMD-ARG-DATA）：
 *   命令(CMD) -> DC=1
 *   参数(ARG) -> DC=1
 *   数据(DATA) -> DC=0
 * 适用于大多数SPI接口的显示控制器。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_110(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SetDC(u8x8, 1);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SetDC(u8x8, 1);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_byte_SetDC(u8x8, 0);
      //u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief GU800控制器CAD 110模式回调函数
 *
 * 与标准CAD110类似，但每个字节都需要独立的传输开始/结束。
 * 适用于GU800系列显示控制器。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_gu800_cad_110(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *data;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SetDC(u8x8, 1);
      u8x8_byte_StartTransfer(u8x8);
      u8x8_byte_SendByte(u8x8, arg_int);
      u8x8_byte_EndTransfer(u8x8);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SetDC(u8x8, 1);
      u8x8_byte_StartTransfer(u8x8);
      u8x8_byte_SendByte(u8x8, arg_int);
      u8x8_byte_EndTransfer(u8x8);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_byte_SetDC(u8x8, 0);
      data = (uint8_t *)arg_ptr;
      while( arg_int > 0 )
      {
        u8x8_byte_StartTransfer(u8x8);
        u8x8_byte_SendByte(u8x8, *data);
        u8x8_byte_EndTransfer(u8x8);
        data++;
        arg_int--;
      }
      break;
    case U8X8_MSG_CAD_INIT:
      u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
      break;
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      break;
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief CAD 100模式回调函数（T6963兼容）
 *
 * DC信号分配规则：
 *   命令(CMD) -> DC=1
 *   参数(ARG) -> DC=0
 *   数据(DATA) -> DC=0
 * 适用于T6963等显示控制器。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_100(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SetDC(u8x8, 1);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SetDC(u8x8, 0);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_byte_SetDC(u8x8, 0);
      //u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief CAD 001模式回调函数
 *
 * DC信号分配规则：
 *   命令(CMD) -> DC=0
 *   参数(ARG) -> DC=0
 *   数据(DATA) -> DC=1
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_001(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SetDC(u8x8, 0);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SetDC(u8x8, 0);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_byte_SetDC(u8x8, 1);
      //u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief CAD 011模式回调函数
 *
 * DC信号分配规则：
 *   命令(CMD) -> DC=0
 *   参数(ARG) -> DC=1
 *   数据(DATA) -> DC=1
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_011(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SetDC(u8x8, 0);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SetDC(u8x8, 1);
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_byte_SetDC(u8x8, 1);
      //u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr);
      //break;
      /* fall through */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief ST7920 SPI模式CAD回调函数
 *
 * ST7920的SPI通信协议比较特殊：
 * - 每个字节分为两次SPI传输（高4位和低4位）
 * - 命令前缀: 0xF8
 * - 数据前缀: 0xFA
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数（数据缓冲区）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_st7920_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *data;
  uint8_t b;
  uint8_t i;
  static uint8_t buf[16];
  uint8_t *ptr;
  
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_SendByte(u8x8, 0x0f8);
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
      u8x8_byte_SendByte(u8x8, arg_int & 0x0f0);
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
      u8x8_byte_SendByte(u8x8, arg_int << 4);
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SendByte(u8x8, 0x0f8);
      u8x8_byte_SendByte(u8x8, arg_int & 0x0f0);
      u8x8_byte_SendByte(u8x8, arg_int << 4);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
    
      u8x8_byte_SendByte(u8x8, 0x0fa);
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);

      /* this loop should be optimized: multiple bytes should be sent */
      /* u8x8_byte_SendBytes(u8x8, arg_int, arg_ptr); */
      data = (uint8_t *)arg_ptr;
    
      /* the following loop increases speed by 20% */
      while( arg_int >= 8 )
      {
	i = 8;
	ptr = buf;
	do
	{
	  b = *data++;
	  *ptr++= b & 0x0f0;
	  b <<= 4;
	  *ptr++= b;
	  i--;
	} while( i > 0 );
	arg_int -= 8;
	u8x8_byte_SendBytes(u8x8, 16, buf); 
      }
      
    
      while( arg_int > 0 )
      {
	b = *data;
	u8x8_byte_SendByte(u8x8, b & 0x0f0);
	u8x8_byte_SendByte(u8x8, b << 4);
	data++;
	arg_int--;
      }
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
      break;
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
      return 0;
  }
  return 1;
}


/**
 * @brief I2C数据传输辅助函数
 *
 * 执行一次完整的I2C数据传输：开始 -> 0x40(数据控制字节) -> 数据 -> 结束。
 * 用于SSD13xx等I2C接口的OLED控制器。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param arg_int 要传输的数据字节数
 * @param arg_ptr 指向数据缓冲区的指针
 */
static void u8x8_i2c_data_transfer(u8x8_t *u8x8, uint8_t arg_int, void *arg_ptr) U8X8_NOINLINE;
static void u8x8_i2c_data_transfer(u8x8_t *u8x8, uint8_t arg_int, void *arg_ptr)
{
    u8x8_byte_StartTransfer(u8x8);                 /* 开始I2C传输 */
    u8x8_byte_SendByte(u8x8, 0x040);               /* 发送数据控制字节 0x40 */
    u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, arg_ptr);  /* 发送数据 */
    u8x8_byte_EndTransfer(u8x8);                   /* 结束I2C传输 */
}

/**
 * @brief SSD13xx I2C模式CAD回调函数（经典版）
 *
 * 适用于SSD1306、SSD1309等SSD13xx系列OLED控制器的I2C通信。
 * I2C控制字节：
 *   0x00 - 命令
 *   0x40 - 数据
 *
 * 经典版特点：每个命令/参数都有独立的I2C起始/停止条件。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_ssd13xx_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *p;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
    case U8X8_MSG_CAD_SEND_ARG:
      /* 7 Nov 2016: Can this be improved?  */
      //u8x8_byte_SetDC(u8x8, 0);
      u8x8_byte_StartTransfer(u8x8);
      //u8x8_byte_SendByte(u8x8, u8x8_GetI2CAddress(u8x8));
      u8x8_byte_SendByte(u8x8, 0x000);
      u8x8_byte_SendByte(u8x8, arg_int);
      u8x8_byte_EndTransfer(u8x8);      
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      //u8x8_byte_SetDC(u8x8, 1);
    
      /* the FeatherWing OLED with the 32u4 transfer of long byte */
      /* streams was not possible. This is broken down to */
      /* smaller streams, 32 seems to be the limit... */
      /* I guess this is related to the size of the Wire buffers in Arduino */
      /* Unfortunately, this can not be handled in the byte level drivers, */
      /* so this is done here. Even further, only 24 bytes will be sent, */
      /* because there will be another byte (DC) required during the transfer */
      p = arg_ptr;
       while( arg_int > 24 )
      {
	u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      u8x8_i2c_data_transfer(u8x8, arg_int, p);
      break;
    case U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      /* cad transfer commands are ignored */
      break;
    default:
      return 0;
  }
  return 1;
}


/**
 * @brief SSD13xx I2C模式CAD回调函数（快速版）
 *
 * 与经典版相比，快速版减少了I2C传输的开始/停止次数，
 * 提高了约4-6%的通信效率（参考issue #735）。
 *
 * 优化策略：命令和参数在同一次I2C传输中发送，
 * 只有在发送数据或命令切换时才结束传输。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_ssd13xx_fast_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;
  uint8_t *p;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      /* improved version, takeover from ld7032 */
      /* assumes, that the args of a command is not longer than 31 bytes */
      /* speed improvement is about 4% compared to the classic version */
      if ( in_transfer != 0 )
	 u8x8_byte_EndTransfer(u8x8); 
      
      u8x8_byte_StartTransfer(u8x8);
      u8x8_byte_SendByte(u8x8, 0x000);	/* cmd byte for ssd13xx controller */
      u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      /* lightning version: can replace the improved version from above */
      /* the drawback of the lightning version is this: The complete init sequence */
      /* must fit into the 32 byte Arduino Wire buffer, which might not always be the case */
      /* speed improvement is about 6% compared to the classic version */
      // if ( in_transfer == 0 )
	// {
	//   u8x8_byte_StartTransfer(u8x8);
	//   u8x8_byte_SendByte(u8x8, 0x000);	/* cmd byte for ssd13xx controller */
	//   in_transfer = 1;
	// }
	//u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SendByte(u8x8, arg_int);
      break;      
    case U8X8_MSG_CAD_SEND_DATA:
      if ( in_transfer != 0 )
	u8x8_byte_EndTransfer(u8x8); 
      
    
      /* the FeatherWing OLED with the 32u4 transfer of long byte */
      /* streams was not possible. This is broken down to */
      /* smaller streams, 32 seems to be the limit... */
      /* I guess this is related to the size of the Wire buffers in Arduino */
      /* Unfortunately, this can not be handled in the byte level drivers, */
      /* so this is done here. Even further, only 24 bytes will be sent, */
      /* because there will be another byte (DC) required during the transfer */
      p = arg_ptr;
       while( arg_int > 24 )
      {
	u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      u8x8_i2c_data_transfer(u8x8, arg_int, p);
      in_transfer = 0;
      break;
    case U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;
      break;
    case U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	u8x8_byte_EndTransfer(u8x8); 
      in_transfer = 0;
      break;
    default:
      return 0;
  }
  return 1;
}



/**
 * @brief ST75256 I2C模式CAD回调函数
 *
 * 基于SSD13xx驱动修改，适用于ST75256 LCD控制器。
 * 与SSD13xx的区别：参数(ARG)使用数据控制字节0x40发送。
 * I2C控制字节：
 *   0x00 - 命令
 *   0x40 - 参数和数据
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_st75256_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t *p;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_byte_StartTransfer(u8x8);
      u8x8_byte_SendByte(u8x8, 0x000);
      u8x8_byte_SendByte(u8x8, arg_int);
      u8x8_byte_EndTransfer(u8x8);      
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_StartTransfer(u8x8);
      u8x8_byte_SendByte(u8x8, 0x040);
      u8x8_byte_SendByte(u8x8, arg_int);
      u8x8_byte_EndTransfer(u8x8);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      /* see ssd13xx driver */
      p = arg_ptr;
       while( arg_int > 24 )
      {
	u8x8_i2c_data_transfer(u8x8, 24, p);
	arg_int-=24;
	p+=24;
      }
      u8x8_i2c_data_transfer(u8x8, arg_int, p);
      break;
    case U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;	/* ST75256, often this is 0x07e */
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
      /* cad transfer commands are ignored */
      break;
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief LD7032 I2C模式CAD回调函数
 *
 * 适用于LD7032 OLED控制器的I2C通信。
 * LD7032的数据写入控制字节为0x08。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_ld7032_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;
  uint8_t *p;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      if ( in_transfer != 0 )
	u8x8_byte_EndTransfer(u8x8); 
      u8x8_byte_StartTransfer(u8x8);
      u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_byte_SendByte(u8x8, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      //u8x8_byte_SetDC(u8x8, 1);
    
      /* the FeatherWing OLED with the 32u4 transfer of long byte */
      /* streams was not possible. This is broken down to */
      /* smaller streams, 32 seems to be the limit... */
      /* I guess this is related to the size of the Wire buffers in Arduino */
      /* Unfortunately, this can not be handled in the byte level drivers, */
      /* so this is done here. Even further, only 24 bytes will be sent, */
      /* because there will be another byte (DC) required during the transfer */
      p = arg_ptr;
       while( arg_int > 24 )
      {
	u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, 24, p);
	arg_int-=24;
	p+=24;
	u8x8_byte_EndTransfer(u8x8); 
	u8x8_byte_StartTransfer(u8x8);
	u8x8_byte_SendByte(u8x8, 0x08);	/* data write for LD7032 */
      }
      u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, p);
      break;
    case U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x060;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;
      break;
    case U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	u8x8_byte_EndTransfer(u8x8); 
      break;
    default:
      return 0;
  }
  return 1;
}

/**
 * @brief UC16xx I2C模式CAD回调函数
 *
 * 适用于UC16xx系列LCD控制器（如UC1601、UC1604等）的I2C通信。
 * 特点：DC位编码在I2C地址的低2位中：
 *   地址低2位=00: 命令模式
 *   地址低2位=10: 数据模式
 * CAD结构为CAD001（命令和参数都用命令模式发送）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_uc16xx_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;	
  static uint8_t is_data = 0;
  uint8_t *p;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
    case U8X8_MSG_CAD_SEND_ARG:
      if ( in_transfer != 0 )
      {
	if ( is_data != 0 )
	{
	  /* transfer mode is active, but data transfer */
	  u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  u8x8_SetI2CAddress( u8x8, u8x8_GetI2CAddress(u8x8)&0x0fc );
	  u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	u8x8_SetI2CAddress( u8x8, u8x8_GetI2CAddress(u8x8)&0x0fc );
	u8x8_byte_StartTransfer(u8x8);
      }
      u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      // is_data = 0;  // 20 Jun 2021: I assume that this is missing here
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      if ( in_transfer != 0 )
      {
	if ( is_data == 0 )
	{
	  /* transfer mode is active, but data transfer */
	  u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  u8x8_SetI2CAddress( u8x8, (u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	  u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	u8x8_SetI2CAddress( u8x8, (u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	u8x8_byte_StartTransfer(u8x8);
      }
      in_transfer = 1;
      // is_data = 1;  // 20 Jun 2021: I assume that this is missing here
      
      p = arg_ptr;
      while( arg_int > 24 )
      {
	u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, 24, p);
	arg_int-=24;
	p+=24;
	u8x8_byte_EndTransfer(u8x8); 
	u8x8_byte_StartTransfer(u8x8);
      }
      u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, p);
      
      break;
    case U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x070;
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;    
      /* actual start is delayed, because we do not whether this is data or cmd transfer */
      break;
    case U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	u8x8_byte_EndTransfer(u8x8);
      in_transfer = 0;
      break;
    default:
      return 0;
  }
  return 1;
}


/**
 * @brief UC1638 I2C模式CAD回调函数
 *
 * 与UC16xx I2C类似，但CAD结构为CAD011：
 *   命令(CMD) -> 地址低2位=00
 *   参数(ARG) -> 地址低2位=10（与数据相同）
 *   数据(DATA) -> 地址低2位=10
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param msg 消息类型
 * @param arg_int 整数参数
 * @param arg_ptr 指针参数
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_cad_uc1638_i2c(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  static uint8_t in_transfer = 0;	
  static uint8_t is_data = 0;
  uint8_t *p;
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      if ( in_transfer != 0 )
      {
	if ( is_data != 0 )
	{
	  /* transfer mode is active, but data transfer */
	  u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  u8x8_SetI2CAddress( u8x8, u8x8_GetI2CAddress(u8x8)&0x0fc );
	  u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	u8x8_SetI2CAddress( u8x8, u8x8_GetI2CAddress(u8x8)&0x0fc );
	u8x8_byte_StartTransfer(u8x8);
      }
      u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      is_data = 0;
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      if ( in_transfer != 0 )
      {
	if ( is_data == 0 )
	{
	  /* transfer mode is active, but data transfer */
	  u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  u8x8_SetI2CAddress( u8x8, (u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	  u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	u8x8_SetI2CAddress( u8x8, (u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	u8x8_byte_StartTransfer(u8x8);
      }
      u8x8_byte_SendByte(u8x8, arg_int);
      in_transfer = 1;
      is_data = 1;
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      if ( in_transfer != 0 )
      {
	if ( is_data == 0 )
	{
	  /* transfer mode is active, but data transfer */
	  u8x8_byte_EndTransfer(u8x8); 
	  /* clear the lowest two bits of the adr */
	  u8x8_SetI2CAddress( u8x8, (u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	  u8x8_byte_StartTransfer(u8x8); 
	}
      }
      else
      {
	/* clear the lowest two bits of the adr */
	u8x8_SetI2CAddress( u8x8, (u8x8_GetI2CAddress(u8x8)&0x0fc)|2 );
	u8x8_byte_StartTransfer(u8x8);
      }
      in_transfer = 1;
      is_data = 1;
      
      p = arg_ptr;
      while( arg_int > 24 )
      {
	u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, 24, p);
	arg_int-=24;
	p+=24;
	u8x8_byte_EndTransfer(u8x8); 
	u8x8_byte_StartTransfer(u8x8);
      }
      u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, p);
      
      break;
    case U8X8_MSG_CAD_INIT:
      /* apply default i2c adr if required so that the start transfer msg can use this */
      if ( u8x8->i2c_address == 255 )
	u8x8->i2c_address = 0x078;  /* see also https://github.com/olikraus/u8g2/issues/371 for a discussion on this value */
      return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
      in_transfer = 0;    
      /* actual start is delayed, because we do not whether this is data or cmd transfer */
      break;
    case U8X8_MSG_CAD_END_TRANSFER:
      if ( in_transfer != 0 )
	u8x8_byte_EndTransfer(u8x8);
      in_transfer = 0;
      break;
    default:
      return 0;
  }
  return 1;
}
