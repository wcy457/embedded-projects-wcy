/*

  u8x8_d_ssd1306_128x64_noname.c

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
 * 文件: u8x8_d_ssd1306_128x64_noname.c
 * 功能: SSD1306/SSD1312/SH1106 OLED显示屏驱动程序
 *
 * 本文件实现了u8x8库中针对128x64像素OLED显示屏的底层驱动，支持以下显示芯片:
 *   - SSD1306: 最常见的OLED驱动芯片，支持I2C和SPI通信
 *   - SSD1312: SSD1306的变体，扫描方向略有不同
 *   - SH1106:  与SSD1306兼容但内部RAM组织不同的OLED驱动芯片
 *
 * 主要功能:
 *   - 显示屏初始化序列配置(时钟、复用率、对比度、电荷泵等)
 *   - 电源管理模式(开启/关闭显示)
 *   - 显示翻转模式(水平/垂直镜像)
 *   - 对比度调节
 *   - 显存数据传输(绘制图块)
 *
 * 在STM32智能手表项目中，此文件负责驱动0.96寸或1.3寸OLED显示屏。
 */

#include "u8x8.h"



/* SSD1306 128x64 通用初始化序列 - 适用于大多数小型OLED显示屏 */
static const uint8_t u8x8_d_ssd1306_128x64_noname_init_seq[] = {
    
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  U8X8_C(0x0ae),		                /* display off */
  U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  U8X8_CA(0x0d3, 0x000),		/* display offset */
  U8X8_C(0x040),		                /* set display start line to 0 */
  U8X8_CA(0x08d, 0x014),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable, SSD1306 only, should be removed for SH1106 */
  U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  U8X8_CA(0x081, 0x0cf), 		/* [2] set contrast control */
  U8X8_CA(0x0d9, 0x0f1), 		/* [2] pre-charge period 0x022/f1*/
  U8X8_CA(0x0db, 0x040), 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  U8X8_C(0x02e),				/* Deactivate scroll */ 
  U8X8_C(0x0a4),				/* output ram to display */
  U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/*
 * SSD1306 128x64 VCOMH=0 初始化序列
 * 此配置最大化亮度调节范围(setContrast可调范围最大)
 * 缺点: VCOMH取消选择电平设为0，对某些OLED模块兼容性不佳(参见issue #116)
 */
static const uint8_t u8x8_d_ssd1306_128x64_vcomh0_init_seq[] = {
    
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  U8X8_C(0x0ae),		                /* display off */
  U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  U8X8_CA(0x0d3, 0x000),		/* display offset */
  U8X8_C(0x040),		                /* set display start line to 0 */
  U8X8_CA(0x08d, 0x014),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable */
  U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */
  U8X8_CA(0x081, 0x0ef),		/* [2] set contrast control,  */
  U8X8_CA(0x0d9, 0x0a1),		/* [2] pre-charge period 0x022/f1*/
  U8X8_CA(0x0db, 0x000),		/* vcomh deselect level 0x000 .. 0x070, low nibble always 0 */
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  
  U8X8_C(0x02e),				/* Deactivate scroll */ 
  U8X8_C(0x0a4),				/* output ram to display */
  U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};


/*
 * SSD1306 128x64 替代配置初始化序列 (alt0)
 * 与通用初始化序列相同，但0x0da寄存器的bit4设为0
 * 此设置禁用了替代COM引脚配置，适用于特定硬件连接方式
 */
static const uint8_t u8x8_d_ssd1306_128x64_alt0_init_seq[] = {
    
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  U8X8_C(0x0ae),		                /* display off */
  U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  U8X8_CA(0x0d3, 0x000),		/* display offset */
  U8X8_C(0x040),		                /* set display start line to 0 */
  U8X8_CA(0x08d, 0x014),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable, SSD1306 only, should be removed for SH1106 */
  U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  // Flipmode
  // U8X8_C(0x0a0),				/* segment remap a0/a1*/
  // U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  U8X8_CA(0x0da, 0x002),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  U8X8_CA(0x081, 0x0cf), 		/* [2] set contrast control */
  U8X8_CA(0x0d9, 0x0f1), 		/* [2] pre-charge period 0x022/f1*/
  U8X8_CA(0x0db, 0x040), 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  U8X8_C(0x02e),				/* Deactivate scroll */ 
  U8X8_C(0x0a4),				/* output ram to display */
  U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};



/*
 * SH1106 128x64 Winstar显示屏专用初始化序列
 * 参见issue #316和论坛讨论: https://www.mikrocontroller.net/topic/431371
 * SH1106与SSD1306的主要区别在于内置DC-DC转换器配置和电压设置
 */
static const uint8_t u8x8_d_sh1106_128x64_winstar_init_seq[] = {
    
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  U8X8_C(0xae),                 // Display OFF/ON: off (POR = 0xae)
  U8X8_C(0xa4),                 // Set Entire Display OFF/ON: off (POR = 0xa4)
  U8X8_CA(0xd5, 0x50),       // Divide Ratio/Oscillator FrequencyData Set: divide ratio = 1 (POR = 1), Oscillator Frequency = +/- 0% (POR = +/- 0%)
  U8X8_CA(0xa8, 0x3f),       // Multiplex Ratio Data Set: 64 (POR = 0x3f, 64)
  U8X8_CA(0xd3, 0x00),       // Display OffsetData Set: 0 (POR = 0x00)
  U8X8_C(0x40),                 // Set Display Start Line: 0  
  U8X8_CA(0xad, 0x8b),       // DC-DC ON/OFF Mode Set: Built-in DC-DC is used, Normal Display (POR = 0x8b)
  U8X8_CA(0xd9, 0x22),       // Dis-charge/Pre-charge PeriodData Set: pre-charge 2 DCLKs, dis-charge 2 DCLKs (POR = 0x22, pre-charge 2 DCLKs, dis-charge 2 DCLKs)
  U8X8_CA(0xdb, 0x35),       // VCOM Deselect LevelData Set: 0,770V (POR = 0x35, 0,770 V)
  U8X8_C(0x32), // Set Pump voltage value: 8,0 V (POR = 0x32, 8,0 V)
  U8X8_CA(0x81, 0xff),       // Contrast Data Register Set: 255 (large) (POR = 0x80)
  U8X8_C(0x0a6),			// Set Normal/Reverse Display: normal (POR = 0xa6)
  U8X8_CA(0x0da, 0x012),		// com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) 
      
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/* SSD1312 128x64 初始化序列 - SSD1306的变体，扫描方向默认为正向(C0) */
static const uint8_t u8x8_d_ssd1312_128x64_noname_init_seq[] = {
    
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  
  
  U8X8_C(0x0ae),		                /* display off */
  U8X8_CA(0x0d5, 0x080),		/* clock divide ratio (0x00=1) and oscillator frequency (0x8) */
  U8X8_CA(0x0a8, 0x03f),		/* multiplex ratio */
  U8X8_CA(0x0d3, 0x000),		/* display offset */
  U8X8_C(0x040),		                /* set display start line to 0 */
  U8X8_CA(0x08d, 0x014),		/* [2] charge pump setting (p62): 0x014 enable, 0x010 disable, SSD1306 only, should be removed for SH1106 */
  U8X8_CA(0x020, 0x000),		/* horizontal addressing mode */
  
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  
  U8X8_CA(0x0da, 0x012),		/* com pin HW config, sequential com pin config (bit 4), disable left/right remap (bit 5) */

  U8X8_CA(0x081, 0x0cf), 		/* [2] set contrast control */
  U8X8_CA(0x0d9, 0x0f1), 		/* [2] pre-charge period 0x022/f1*/
  U8X8_CA(0x0db, 0x040), 		/* vcomh deselect level */  
  // if vcomh is 0, then this will give the biggest range for contrast control issue #98
  // restored the old values for the noname constructor, because vcomh=0 will not work for all OLEDs, #116
  
  U8X8_C(0x02e),				/* Deactivate scroll */ 
  U8X8_C(0x0a4),				/* output ram to display */
  U8X8_C(0x0a6),				/* none inverted normal display mode */
    
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};


/* 电源管理模式0: 开启显示(正常工作模式) */
static const uint8_t u8x8_d_ssd1306_128x64_noname_powersave0_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0af),		                /* display on */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/* 电源管理模式1: 关闭显示(省电模式，OLED不消耗电流) */
static const uint8_t u8x8_d_ssd1306_128x64_noname_powersave1_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0ae),		                /* display off */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/* 翻转模式0: 正常显示方向(segment重映射A1，扫描方向反向C8) */
static const uint8_t u8x8_d_ssd1306_128x64_noname_flip0_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/* 翻转模式1: 翻转180度显示(segment重映射A0，扫描方向正向C0) */
static const uint8_t u8x8_d_ssd1306_128x64_noname_flip1_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a0),				/* segment remap a0/a1*/
  U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/* SSD1312 翻转模式0: 正常显示方向(注意与SSD1306的扫描方向相反) */
static const uint8_t u8x8_d_ssd1312_128x64_noname_flip0_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a1),				/* segment remap a0/a1*/
  U8X8_C(0x0c0),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/* SSD1312 翻转模式1: 翻转180度显示 */
static const uint8_t u8x8_d_ssd1312_128x64_noname_flip1_seq[] = {
  U8X8_START_TRANSFER(),             	/* enable chip, delay is part of the transfer start */
  U8X8_C(0x0a0),				/* segment remap a0/a1*/
  U8X8_C(0x0c8),				/* c0: scan dir normal, c8: reverse */
  U8X8_END_TRANSFER(),             	/* disable chip */
  U8X8_END()             			/* end of sequence */
};

/*
 * 函数: u8x8_d_ssd1306_sh1106_generic
 * 功能: SSD1306/SH1106通用显示驱动消息处理函数
 *       处理所有SSD1306和SH1106芯片共用的显示控制消息
 *
 * 参数:
 *   u8x8    - u8x8显示结构体指针，包含显示屏的所有配置信息
 *   msg     - 消息类型，决定要执行的操作(电源控制、翻转、对比度、绘制等)
 *   arg_int - 整型参数，根据msg类型有不同含义
 *   arg_ptr - 指针参数，通常指向图块(tile)数据
 *
 * 返回值:
 *   1 - 消息已处理
 *   0 - 消息未处理(不支持的消息类型)
 */
static uint8_t u8x8_d_ssd1306_sh1106_generic(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  uint8_t x, c;       /* x: 像素列坐标, c: 图块计数 */
  uint8_t *ptr;       /* 指向图块像素数据的指针 */
  switch(msg)
  {
    /* 以下两个消息由调用函数处理，此处不处理
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_ssd1306_128x64_noname_display_info);
      break;
    */
    /* 以下两个消息由调用函数处理，此处不处理
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_init_seq);
      break;
    */
    case U8X8_MSG_DISPLAY_SET_POWER_SAVE:           /* 设置电源管理模式 */
      if ( arg_int == 0 )
	u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_powersave0_seq);  /* arg_int=0: 开启显示 */
      else
	u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_powersave1_seq);  /* arg_int!=0: 关闭显示(省电) */
      break;
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:            /* 设置显示翻转模式 */
      if ( arg_int == 0 )
      {
	u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_flip0_seq);       /* 正常方向 */
	u8x8->x_offset = u8x8->display_info->default_x_offset;                     /* 恢复默认X偏移 */
      }
      else
      {
	u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_flip1_seq);       /* 翻转180度 */
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;                    /* 使用翻转模式的X偏移 */
      }
      break;
#ifdef U8X8_WITH_SET_CONTRAST
    case U8X8_MSG_DISPLAY_SET_CONTRAST:             /* 设置显示对比度 */
      u8x8_cad_StartTransfer(u8x8);                 /* 开始SPI/I2C传输 */
      u8x8_cad_SendCmd(u8x8, 0x081 );              /* 发送对比度控制命令(0x81) */
      u8x8_cad_SendArg(u8x8, arg_int );	            /* 发送对比度值，SSD1306范围: 0~255 */
      u8x8_cad_EndTransfer(u8x8);                   /* 结束传输 */
      break;
#endif
    case U8X8_MSG_DISPLAY_DRAW_TILE:                /* 绘制图块(tile) - 核心绘图功能 */
      u8x8_cad_StartTransfer(u8x8);                 /* 开始数据传输 */
      x = ((u8x8_tile_t *)arg_ptr)->x_pos;          /* 获取图块的X位置(以tile为单位) */
      x *= 8;                                       /* 转换为像素坐标(每个tile宽8像素) */
      x += u8x8->x_offset;                          /* 加上硬件X偏移量 */

      u8x8_cad_SendCmd(u8x8, 0x040 );	            /* 设置显示起始行为0 */

      /* 设置列地址: 高4位命令0x10 | (x>>4)，低4位命令0x00 | (x&15) */
      u8x8_cad_SendCmd(u8x8, 0x010 | (x>>4) );
      u8x8_cad_SendArg(u8x8, 0x000 | ((x&15)));				                    /* 设置列地址低4位 */
      u8x8_cad_SendArg(u8x8, 0x0b0 | (((u8x8_tile_t *)arg_ptr)->y_pos));	    /* 设置页地址(行地址) 0xB0~0xB7 */


      do
      {
	c = ((u8x8_tile_t *)arg_ptr)->cnt;             /* 获取要绘制的tile数量 */
	ptr = ((u8x8_tile_t *)arg_ptr)->tile_ptr;      /* 获取像素数据指针 */
	u8x8_cad_SendData(u8x8, c*8, ptr); 	        /* 发送像素数据(每个tile 8字节) */
	                                               /* 注意: SendData单次不能超过255字节 */
	/*
	do
	{
	  u8x8_cad_SendData(u8x8, 8, ptr);              // 逐个tile发送(备用方案)
	  ptr += 8;
	  c--;
	} while( c > 0 );
	*/
	arg_int--;                                     /* 减少剩余图块行数 */
      } while( arg_int > 0 );                        /* 循环直到所有图块行发送完毕 */

      u8x8_cad_EndTransfer(u8x8);                    /* 结束数据传输 */
      break;
    default:
      return 0;                                      /* 未处理的消息返回0 */
  }
  return 1;                                          /* 已处理的消息返回1 */
}


/*
 * SSD1306 128x64 显示屏硬件参数结构体
 * 定义了显示屏的电气特性、时序参数和尺寸信息
 * 用于驱动程序初始化和通信配置
 */
static const u8x8_display_info_t u8x8_ssd1306_128x64_noname_display_info =
{
  /* chip_enable_level = */ 0,              /* 片选有效电平: 低电平选中 */
  /* chip_disable_level = */ 1,             /* 片选无效电平: 高电平取消选中 */

  /* post_chip_enable_wait_ns = */ 20,      /* 片选使能后等待时间(ns) */
  /* pre_chip_disable_wait_ns = */ 10,      /* 片选取消前等待时间(ns) */
  /* reset_pulse_width_ms = */ 100, 	     /* 复位脉冲宽度(ms)，SSD1306规格: 3us */
  /* post_reset_wait_ms = */ 100,           /* 复位后等待时间(ms)，远东OLED模块需要更长启动时间 */
  /* sda_setup_time_ns = */ 50,		     /* SDA建立时间(ns)，SSD1306规格: 15ns */
  /* sck_pulse_width_ns = */ 50,	         /* SCK脉冲宽度(ns)，SSD1306规格: 20ns */
  /* sck_clock_hz = */ 8000000UL,	         /* SPI时钟频率(Hz): 8MHz */
  /* spi_mode = */ 0,		                 /* SPI模式: 模式0(上升沿采样) */
  /* i2c_bus_clock_100kHz = */ 4,           /* I2C总线时钟: 400kHz(4*100kHz) */
  /* data_setup_time_ns = */ 40,            /* 数据建立时间(ns) */
  /* write_pulse_width_ns = */ 150,	     /* 写脉冲宽度(ns)，SSD1306周期: 300ns，取一半 */
  /* tile_width = */ 16,                    /* 水平方向tile数量: 128/8=16 */
  /* tile_height = */ 8,                    /* 垂直方向tile数量: 64/8=8 */
  /* default_x_offset = */ 0,              /* 默认X偏移量(像素) */
  /* flipmode_x_offset = */ 0,             /* 翻转模式X偏移量(像素) */
  /* pixel_width = */ 128,                  /* 显示屏宽度(像素) */
  /* pixel_height = */ 64                   /* 显示屏高度(像素) */
};

/*
 * 函数: u8x8_d_ssd1306_128x64_noname
 * 功能: SSD1306 128x64 无名(通用)OLED显示屏驱动函数
 *       这是最常用的SSD1306驱动入口，适用于大多数128x64 OLED模块
 *
 * 参数:
 *   u8x8    - u8x8显示结构体指针
 *   msg     - 消息类型(U8X8_MSG_DISPLAY_INIT/SETUP_MEMORY等)
 *   arg_int - 整型参数
 *   arg_ptr - 指针参数
 *
 * 返回值:
 *   1 - 消息已处理
 *   0 - 消息未处理
 */
uint8_t u8x8_d_ssd1306_128x64_noname(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{

  /* 首先尝试通用处理(电源、翻转、对比度、绘制等) */
  if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;

  switch(msg)
  {
    case U8X8_MSG_DISPLAY_INIT:                        /* 显示初始化消息 */
      u8x8_d_helper_display_init(u8x8);                /* 执行通用初始化 */
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_init_seq);    /* 发送SSD1306初始化命令序列 */
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:                /* 设置显示内存配置 */
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_ssd1306_128x64_noname_display_info);  /* 绑定显示参数 */
      break;
    default:
      return 0;                                        /* 未处理的消息 */
  }
  return 1;
}

/*
 * 函数: u8x8_d_ssd1312_128x64_noname
 * 功能: SSD1312 128x64 OLED显示屏驱动函数
 *       SSD1312与SSD1306类似，但翻转模式的扫描方向不同
 *
 * 参数:
 *   u8x8    - u8x8显示结构体指针
 *   msg     - 消息类型
 *   arg_int - 整型参数
 *   arg_ptr - 指针参数
 *
 * 返回值: 始终返回1
 */
uint8_t u8x8_d_ssd1312_128x64_noname(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  switch(msg)
  {
    case U8X8_MSG_DISPLAY_SET_FLIP_MODE:               /* SSD1312专用翻转处理(扫描方向与SSD1306相反) */
      if ( arg_int == 0 )
      {
	u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1312_128x64_noname_flip0_seq);   /* 正常方向 */
	u8x8->x_offset = u8x8->display_info->default_x_offset;
      }
      else
      {
	u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1312_128x64_noname_flip1_seq);   /* 翻转方向 */
	u8x8->x_offset = u8x8->display_info->flipmode_x_offset;
      }
      break;
    case U8X8_MSG_DISPLAY_INIT:                        /* 显示初始化 */
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1312_128x64_noname_init_seq);    /* 发送SSD1312专用初始化序列 */
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:                /* 设置显示内存 */
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_ssd1306_128x64_noname_display_info);  /* 复用SSD1306的显示参数 */
      break;
    default:
      if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )  /* 其他消息交给通用处理 */
        return 1;
  }
  return 1;
}



/*
 * 函数: u8x8_d_ssd1306_128x64_vcomh0
 * 功能: SSD1306 128x64 VCOMH=0配置的驱动函数
 *       使用VCOMH=0配置可获得最大的对比度调节范围
 *       但可能与某些OLED模块不兼容
 *
 * 参数: 同u8x8_d_ssd1306_128x64_noname
 * 返回值: 1-已处理, 0-未处理
 */
uint8_t u8x8_d_ssd1306_128x64_vcomh0(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* 先尝试通用处理 */
  if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;

  switch(msg)
  {
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_vcomh0_init_seq);    /* 使用VCOMH=0的初始化序列 */
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_ssd1306_128x64_noname_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}

/*
 * 函数: u8x8_d_ssd1306_128x64_alt0
 * 功能: SSD1306 128x64 替代COM配置的驱动函数
 *       禁用替代COM引脚配置(0x0da bit4=0)，适用于特定硬件连接
 *
 * 参数: 同u8x8_d_ssd1306_128x64_noname
 * 返回值: 1-已处理, 0-未处理
 */
uint8_t u8x8_d_ssd1306_128x64_alt0(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* 先尝试通用处理 */
  if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;

  switch(msg)
  {
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_alt0_init_seq);    /* 使用替代配置的初始化序列 */
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_ssd1306_128x64_noname_display_info);
      break;
    default:
      return 0;
  }
  return 1;
}


/*
 * SH1106 128x64 显示屏硬件参数结构体
 * 与SSD1306的主要区别:
 *   - SPI时钟频率较低(4MHz vs 8MHz)
 *   - X偏移量为2(SH1106的RAM起始地址偏移)
 */
static const u8x8_display_info_t u8x8_sh1106_128x64_noname_display_info =
{
  /* chip_enable_level = */ 0,              /* 片选有效电平: 低电平 */
  /* chip_disable_level = */ 1,             /* 片选无效电平: 高电平 */

  /* post_chip_enable_wait_ns = */ 20,      /* 片选使能后等待(ns) */
  /* pre_chip_disable_wait_ns = */ 10,      /* 片选取消前等待(ns) */
  /* reset_pulse_width_ms = */ 100, 	     /* 复位脉冲宽度(ms) */
  /* post_reset_wait_ms = */ 100,           /* 复位后等待(ms) */
  /* sda_setup_time_ns = */ 50,		     /* SDA建立时间(ns) */
  /* sck_pulse_width_ns = */ 50,	         /* SCK脉冲宽度(ns) */
  /* sck_clock_hz = */ 4000000UL,	         /* SPI时钟频率: 4MHz(比SSD1306慢) */
  /* spi_mode = */ 0,		                 /* SPI模式: 模式0(issue #1901从模式3改为0) */
  /* i2c_bus_clock_100kHz = */ 4,           /* I2C时钟: 400kHz */
  /* data_setup_time_ns = */ 40,            /* 数据建立时间(ns) */
  /* write_pulse_width_ns = */ 150,	     /* 写脉冲宽度(ns) */
  /* tile_width = */ 16,                    /* 水平tile数: 16 */
  /* tile_height = */ 8,                    /* 垂直tile数: 8 */
  /* default_x_offset = */ 2,              /* 默认X偏移: 2像素(SH1106 RAM地址偏移) */
  /* flipmode_x_offset = */ 2,             /* 翻转模式X偏移: 2像素 */
  /* pixel_width = */ 128,                  /* 宽度: 128像素 */
  /* pixel_height = */ 64                   /* 高度: 64像素 */
};

/*
 * 函数: u8x8_d_sh1106_128x64_noname
 * 功能: SH1106 128x64 通用OLED显示屏驱动函数
 *       SH1106与SSD1306兼容，但内部RAM组织不同(列地址有2像素偏移)
 *       此函数使用SSD1306的初始化序列(Winstar版本有更好的初始化序列)
 *
 * 参数: 同u8x8_d_ssd1306_128x64_noname
 * 返回值: 1-已处理, 0-未处理
 */
uint8_t u8x8_d_sh1106_128x64_noname(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  /* 先尝试通用处理 */
  if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;

  switch(msg)
  {
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      /* 注意: Winstar版本有更好的初始化序列，此处保留原始序列以保持兼容性 */
      /* 参见: https://www.mikrocontroller.net/topic/431371 */
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_noname_init_seq);
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_sh1106_128x64_noname_display_info);  /* 使用SH1106专用参数 */
      break;
    default:
      return 0;
  }
  return 1;

}

/*
 * 函数: u8x8_d_sh1106_128x64_vcomh0
 * 功能: SH1106 128x64 VCOMH=0配置的驱动函数
 *       使用VCOMH=0以获得最大对比度调节范围
 *
 * 参数: 同u8x8_d_ssd1306_128x64_noname
 * 返回值: 1-已处理, 0-未处理
 */
uint8_t u8x8_d_sh1106_128x64_vcomh0(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;

  switch(msg)
  {
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_ssd1306_128x64_vcomh0_init_seq);    /* VCOMH=0初始化序列 */
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_sh1106_128x64_noname_display_info);
      break;
    default:
      return 0;
  }
  return 1;

}

/*
 * 函数: u8x8_d_sh1106_128x64_winstar
 * 功能: SH1106 128x64 Winstar OLED显示屏专用驱动函数
 *       使用Winstar专用初始化序列，包含DC-DC转换器和电压泵配置
 *       对于Winstar品牌的SH1106 OLED模块兼容性最佳
 *
 * 参数: 同u8x8_d_ssd1306_128x64_noname
 * 返回值: 1-已处理, 0-未处理
 */
uint8_t u8x8_d_sh1106_128x64_winstar(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
  if ( u8x8_d_ssd1306_sh1106_generic(u8x8, msg, arg_int, arg_ptr) != 0 )
    return 1;

  switch(msg)
  {
    case U8X8_MSG_DISPLAY_INIT:
      u8x8_d_helper_display_init(u8x8);
      u8x8_cad_SendSequence(u8x8, u8x8_d_sh1106_128x64_winstar_init_seq);    /* Winstar专用初始化序列 */
      break;
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
      u8x8_d_helper_display_setup_memory(u8x8, &u8x8_sh1106_128x64_noname_display_info);
      break;
    default:
      return 0;
  }
  return 1;

}

