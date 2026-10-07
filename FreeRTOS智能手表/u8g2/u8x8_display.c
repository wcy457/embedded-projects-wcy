/**
 * @file u8x8_display.c
 * @brief u8x8显示抽象层实现
 *
 * 本文件是u8x8库的显示抽象层，主要功能是在显示器上放置8x8像素块（tile）。
 *
 * 主要功能：
 * - 显示内存初始化和配置
 * - 显示控制器初始化（复位、发送初始化序列）
 * - Tile绘制（8x8像素块）
 * - 显示控制（电源管理、翻转模式、对比度等）
 * - 清屏和填充功能
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
  
  
  Abstraction layer for the graphics controller.
  Main goal is the placement of a 8x8 pixel block (tile) on the display.
  
*/


#include "u8x8.h"


/*==========================================*/
/* internal library function */

/**
 * @brief 显示内存设置辅助函数
 *
 * 处理U8X8_MSG_DISPLAY_SETUP_MEMORY消息的标准任务。
 * 在显示回调函数中调用此函数来完成内存配置。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param display_info 指向显示信息结构体的指针（包含时序、分辨率等参数）
 */
void u8x8_d_helper_display_setup_memory(u8x8_t *u8x8, const u8x8_display_info_t *display_info)
{
      u8x8->display_info = display_info;                                    /* 设置显示信息 */
      u8x8->x_offset = u8x8->display_info->default_x_offset;              /* 设置默认X偏移 */
}

/**
 * @brief 显示初始化辅助函数
 *
 * 处理U8X8_MSG_DISPLAY_INIT消息的标准任务。
 * 在显示回调函数中调用此函数来完成初始化。
 * 初始化流程：
 *   1. 初始化GPIO引脚方向和默认电平
 *   2. 初始化CAD（通信）层
 *   3. 执行硬件复位序列
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_d_helper_display_init(u8x8_t *u8x8)
{
      u8x8_gpio_Init(u8x8);              /* 初始化GPIO引脚 */
      u8x8_cad_Init(u8x8);              /* 初始化CAD层（会调用U8X8_MSG_BYTE_INIT） */

      /* 硬件复位序列：高 -> 延时 -> 低 -> 延时 -> 高 -> 延时 */
      u8x8_gpio_SetReset(u8x8, 1);
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, u8x8->display_info->reset_pulse_width_ms);
      u8x8_gpio_SetReset(u8x8, 0);       /* 拉低复位引脚 */
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, u8x8->display_info->reset_pulse_width_ms);
      u8x8_gpio_SetReset(u8x8, 1);       /* 释放复位 */
      u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, u8x8->display_info->post_reset_wait_ms);
}    

/*==========================================*/
/* official functions */

/**
 * @brief 绘制Tile（8x8像素块）
 *
 * 在指定位置绘制一个或多个连续的8x8像素块。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x Tile的X坐标（以tile为单位，即8像素的倍数）
 * @param y Tile的Y坐标（以tile为单位）
 * @param cnt 要绘制的连续Tile数量
 * @param tile_ptr 指向Tile数据的指针（每个Tile 8字节）
 * @return 操作结果（1成功，0失败）
 */
uint8_t u8x8_DrawTile(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t cnt, uint8_t *tile_ptr)
{
  u8x8_tile_t tile;
  tile.x_pos = x;
  tile.y_pos = y;
  tile.cnt = cnt;
  tile.tile_ptr = tile_ptr;
  return u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_DRAW_TILE, 1, (void *)&tile);
}

/**
 * @brief 设置显示内存
 *
 * 发送U8X8_MSG_DISPLAY_SETUP_MEMORY消息来初始化显示内存配置。
 * 此函数应在u8x8_Setup中调用。
 */
void u8x8_SetupMemory(u8x8_t *u8x8)
{
  u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_SETUP_MEMORY, 0, NULL);
}

/**
 * @brief 仅初始化显示接口（不复位、不发送初始化代码）
 *
 * 与u8x8_InitDisplay的区别：
 *                     u8x8_InitInterface    u8x8_InitDisplay
 *   初始化接口              是                      是
 *   复位显示                否                      是
 *   发送初始化代码          否                      是
 *
 * 注意：不要同时调用u8x8_InitInterface和u8x8_InitDisplay。
 * 当显示已经运行但控制器从深度睡眠模式唤醒时，可以只调用此函数。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_InitInterface(u8x8_t *u8x8)
{
  u8x8_gpio_Init(u8x8);              /* 初始化GPIO */
  u8x8_cad_Init(u8x8);              /* 初始化CAD层 */
}

/**
 * @brief 初始化显示控制器
 *
 * 向显示控制器发送初始化消息。显示回调函数会调用u8x8_d_helper_display_init()
 * 完成以下操作：
 *   1. GPIO初始化（设置端口方向）
 *   2. 字节层初始化（作为CAD初始化的一部分）
 *   3. CAD初始化（设置I2C默认地址等）
 *   4. 硬件复位（通常会关闭显示）
 *   5. 发送初始化代码（通常也会关闭显示，Arduino代码稍后会禁用省电模式）
 *
 * 此函数由Arduino的begin()函数调用。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_InitDisplay(u8x8_t *u8x8)
{
  u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_INIT, 0, NULL);
}

/**
 * @brief 设置显示省电模式
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param is_enable 0=禁用省电（正常显示），1=启用省电（关闭显示）
 */
void u8x8_SetPowerSave(u8x8_t *u8x8, uint8_t is_enable)
{
  u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_SET_POWER_SAVE, is_enable, NULL);
}

/**
 * @brief 设置显示翻转模式
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param mode 翻转模式（0=正常，1=水平翻转，2=垂直翻转，3=180度旋转等）
 */
void u8x8_SetFlipMode(u8x8_t *u8x8, uint8_t mode)
{
  u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_SET_FLIP_MODE, mode, NULL);
}

/**
 * @brief 设置显示对比度
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param value 对比度值（0-255）
 */
void u8x8_SetContrast(u8x8_t *u8x8, uint8_t value)
{
  u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_SET_CONTRAST, value, NULL);
}

/**
 * @brief 刷新显示
 *
 * 将缓冲区内容更新到屏幕上（仅对缓冲模式有效）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_RefreshDisplay(u8x8_t *u8x8)
{
  u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_REFRESH, 0, NULL);
}

/**
 * @brief 使用指定Tile清除整个显示
 *
 * 使用指定的8字节Tile图案填充整个显示屏。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param buf 8字节的Tile数据（用于填充整个屏幕）
 */
void u8x8_ClearDisplayWithTile(u8x8_t *u8x8, const uint8_t *buf)
{
  u8x8_tile_t tile;
  uint8_t h;

  tile.x_pos = 0;
  tile.cnt = 1;
  tile.tile_ptr = (uint8_t *)buf;

  h = u8x8->display_info->tile_height;    /* 获取屏幕高度（以tile为单位） */
  tile.y_pos = 0;
  do
  {
    /* 逐行绘制整行Tile */
    u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_DRAW_TILE, u8x8->display_info->tile_width, (void *)&tile);
    tile.y_pos++;
  } while( tile.y_pos < h );
}

/**
 * @brief 清除显示（全黑）
 *
 * 将整个显示屏清零（所有像素熄灭）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_ClearDisplay(u8x8_t *u8x8)
{
  uint8_t buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };   /* 全零Tile */
  u8x8_ClearDisplayWithTile(u8x8, buf);
}

/**
 * @brief 填充显示（全亮）
 *
 * 将整个显示屏填充为全亮（所有像素点亮）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_FillDisplay(u8x8_t *u8x8)
{
  uint8_t buf[8] = { 255, 255, 255, 255, 255, 255, 255, 255 };  /* 全1Tile */
  u8x8_ClearDisplayWithTile(u8x8, buf);
}

/**
 * @brief 清除指定行
 *
 * 将指定行的所有像素清零。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param line 要清除的行号（以tile为单位）
 */
void u8x8_ClearLine(u8x8_t *u8x8, uint8_t line)
{
  uint8_t buf[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  u8x8_tile_t tile;
  if ( line < u8x8->display_info->tile_height )   /* 检查行号是否有效 */
  {
    tile.x_pos = 0;
    tile.y_pos = line;
    tile.cnt = 1;
    tile.tile_ptr = (uint8_t *)buf;
    u8x8->display_cb(u8x8, U8X8_MSG_DISPLAY_DRAW_TILE, u8x8->display_info->tile_width, (void *)&tile);
  }
}
