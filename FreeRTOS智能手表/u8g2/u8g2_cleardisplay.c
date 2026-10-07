/**
 * @file u8g2_cleardisplay.c
 * @brief u8g2库的显示器清屏功能实现文件
 *
 * 本文件提供了清空显示器内容的功能。
 * 通过分页模式的图片循环实现可靠的清屏操作。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 * Copyright (c) 2016, olikraus@gmail.com
 */
#include "u8g2.h"

/**
 * @brief 清空显示器内容
 * @param u8g2  u8g2显示结构体指针
 *
 * 清空屏幕缓冲区和显示器内容，适用于所有u8g2支持的显示器。
 * 使用分页模式的图片循环实现，因为不能在所有情况下使用u8x8函数。
 *
 * 此函数通常在初始化时（u8g2.begin()）调用。
 * 调用后会重置当前tile行号为0，以兼容全屏缓冲模式。
 */
void u8g2_ClearDisplay(u8g2_t *u8g2)
{
  u8g2_FirstPage(u8g2);    // 初始化分页模式
  do {
    /* 空循环：遍历所有页面，每页发送空白内容 */
  } while ( u8g2_NextPage(u8g2) );
  /*
    此函数通常在初始化时调用（u8g2.begin()）。
    但用户可能想在全屏缓冲模式下使用clear和send命令。
    上面的图片循环会修改当前tile行号，导致问题。
    因此这里重置tile行号为0（修复issue #370）。
  */
  u8g2_SetBufferCurrTileRow(u8g2, 0);  // 重置tile行号
}

