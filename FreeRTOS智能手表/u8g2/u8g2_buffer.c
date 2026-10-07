/**
 * @file u8g2_buffer.c
 * @brief u8g2库的显示缓冲区管理实现文件
 *
 * 本文件提供了u8g2图形库的显示缓冲区管理功能，包括：
 * - 清空显示缓冲区
 * - 将缓冲区数据发送到显示器
 * - 分页显示模式的支持（FirstPage/NextPage）
 * - 局部区域更新功能
 * - 缓冲区数据导出为PBM/XBM格式
 *
 * u8g2支持两种显示模式：
 * - 全屏缓冲模式：整个屏幕内容在RAM中缓冲
 * - 分页模式：只缓冲部分行，逐页发送到显示器（节省RAM）
 *
 * 对于STM32等RAM有限的微控制器，分页模式尤为重要。
 *
 * Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)
 * Copyright (c) 2016, olikraus@gmail.com
 */

#include "u8g2.h"
#include <string.h>

/*============================================*/
/**
 * @brief 清空显示缓冲区
 * @param u8g2  u8g2显示结构体指针
 *
 * 将整个显示缓冲区清零，相当于清除屏幕上的所有内容。
 * 缓冲区大小 = tile_width * tile_buf_height * 8 字节
 */
void u8g2_ClearBuffer(u8g2_t *u8g2)
{
  size_t cnt;
  cnt = u8g2_GetU8x8(u8g2)->display_info->tile_width;  // 获取tile宽度
  cnt *= u8g2->tile_buf_height;   // 乘以缓冲区的tile高度
  cnt *= 8;                        // 每个tile有8行，乘以8得到总字节数
  memset(u8g2->tile_buf_ptr, 0, cnt);  // 将缓冲区全部清零
}

/*============================================*/

/**
 * @brief 发送一行tile数据到显示器
 * @param u8g2           u8g2显示结构体指针
 * @param src_tile_row   源缓冲区中的tile行号
 * @param dest_tile_row  目标显示器上的tile行号
 *
 * 从缓冲区中读取指定行的数据，通过u8x8接口发送到显示器。
 */
static void u8g2_send_tile_row(u8g2_t *u8g2, uint8_t src_tile_row, uint8_t dest_tile_row)
{
  uint8_t *ptr;
  uint16_t offset;
  uint8_t w;

  w = u8g2_GetU8x8(u8g2)->display_info->tile_width;  // 获取tile宽度
  offset = src_tile_row;       // 源行号
  ptr = u8g2->tile_buf_ptr;   // 缓冲区起始指针
  offset *= w;                 // 计算字节偏移
  offset *= 8;                 // 每个tile有8像素高
  ptr += offset;               // 移动指针到对应行
  u8x8_DrawTile(u8g2_GetU8x8(u8g2), 0, dest_tile_row, w, ptr);  // 发送数据到显示器
}

/**
 * @brief 将缓冲区数据发送到显示器RAM
 * @param u8g2  u8g2显示结构体指针
 *
 * 将缓冲区中的所有数据逐行发送到显示器。
 * 对于大多数显示器，发送后内容即可见。
 * 某些显示器（如SSD1606电子纸）还需要调用u8x8_RefreshDisplay()才能显示。
 */
static void u8g2_send_buffer(u8g2_t *u8g2) U8X8_NOINLINE;
static void u8g2_send_buffer(u8g2_t *u8g2)
{
  uint8_t src_row;       // 源缓冲区当前行
  uint8_t src_max;       // 源缓冲区最大行数
  uint8_t dest_row;      // 目标显示器当前行
  uint8_t dest_max;      // 目标显示器最大行数

  src_row = 0;
  src_max = u8g2->tile_buf_height;                         // 缓冲区的tile高度
  dest_row = u8g2->tile_curr_row;                          // 当前显示的起始行
  dest_max = u8g2_GetU8x8(u8g2)->display_info->tile_height;  // 显示器的总tile高度

  do
  {
    u8g2_send_tile_row(u8g2, src_row, dest_row);  // 发送一行数据
    src_row++;
    dest_row++;
  } while( src_row < src_max && dest_row < dest_max );  // 直到缓冲区或显示器数据发送完毕
}

/**
 * @brief 发送缓冲区到显示器并刷新显示
 * @param u8g2  u8g2显示结构体指针
 *
 * 与u8g2_send_buffer相同，但还会发送DISPLAY_REFRESH消息。
 * 主要用于SSD1606等电子纸显示器。
 */
void u8g2_SendBuffer(u8g2_t *u8g2)
{
  u8g2_send_buffer(u8g2);                          // 发送缓冲区数据
  u8x8_RefreshDisplay( u8g2_GetU8x8(u8g2) );      // 发送刷新命令
}

/*============================================*/
/**
 * @brief 设置缓冲区当前tile行号
 * @param u8g2  u8g2显示结构体指针
 * @param row   当前tile行号
 *
 * 更新当前缓冲区对应的显示行位置，并刷新维度和页面窗口信息。
 */
void u8g2_SetBufferCurrTileRow(u8g2_t *u8g2, uint8_t row)
{
  u8g2->tile_curr_row = row;                // 设置当前tile行
  u8g2->cb->update_dimension(u8g2);         // 更新显示维度
  u8g2->cb->update_page_win(u8g2);          // 更新页面窗口
}

/**
 * @brief 初始化分页模式，准备绘制第一页
 * @param u8g2  u8g2显示结构体指针
 *
 * 在分页模式下开始绘制前调用此函数。
 * 如果启用了自动页面清空，则先清空缓冲区。
 * 然后将当前行设置为0（第一页）。
 *
 * 典型用法：
 * @code
 * u8g2_FirstPage(&u8g2);
 * do {
 *   // 绘制内容
 * } while(u8g2_NextPage(&u8g2));
 * @endcode
 */
void u8g2_FirstPage(u8g2_t *u8g2)
{
  if ( u8g2->is_auto_page_clear )
  {
    u8g2_ClearBuffer(u8g2);   // 自动清空缓冲区
  }
  u8g2_SetBufferCurrTileRow(u8g2, 0);  // 设置到第一页
}

/**
 * @brief 发送当前页面并切换到下一页
 * @param u8g2  u8g2显示结构体指针
 * @return 返回1表示还有下一页需要绘制，返回0表示所有页面已绘制完毕
 *
 * 在分页模式下，每绘制完一页内容后调用此函数。
 * 函数会将当前缓冲区发送到显示器，然后移动到下一页。
 */
uint8_t u8g2_NextPage(u8g2_t *u8g2)
{
  uint8_t row;
  u8g2_send_buffer(u8g2);   // 发送当前页数据到显示器
  row = u8g2->tile_curr_row;
  row += u8g2->tile_buf_height;   // 计算下一页的起始行
  if ( row >= u8g2_GetU8x8(u8g2)->display_info->tile_height )
  {
    u8x8_RefreshDisplay( u8g2_GetU8x8(u8g2) );  // 所有页发送完毕，刷新显示
    return 0;   // 返回0表示没有更多页
  }
  if ( u8g2->is_auto_page_clear )
  {
    u8g2_ClearBuffer(u8g2);   // 清空缓冲区准备绘制下一页
  }
  u8g2_SetBufferCurrTileRow(u8g2, row);  // 设置到下一页
  return 1;   // 返回1表示还有更多页
}



/*============================================*/
/**
 * @brief 更新显示器的局部区域
 * @param u8g2  u8g2显示结构体指针
 * @param tx    tile列坐标（像素坐标除以8）
 * @param ty    tile行坐标（像素坐标除以8）
 * @param tw    宽度（tile数）
 * @param th    高度（tile数）
 *
 * 仅更新显示器的指定区域，而不是整个屏幕。
 * 这比发送整个缓冲区更高效，适用于频繁更新小区域的场景。
 *
 * @note 限制：
 * - 仅在全屏缓冲模式下可用（分页模式下无效）
 * - 参数使用tile坐标（像素坐标/8）
 * - 不支持显示旋转/镜像
 * - 不适用于电子纸显示器
 */
void u8g2_UpdateDisplayArea(u8g2_t *u8g2, uint8_t  tx, uint8_t ty, uint8_t tw, uint8_t th)
{
  uint16_t page_size;
  uint8_t *ptr;

  /* 检查是否处于全屏缓冲模式 */
  if ( u8g2->tile_buf_height != u8g2_GetU8x8(u8g2)->display_info->tile_height )
    return;  /* 非全屏缓冲模式，直接返回 */

  page_size = u8g2->pixel_buf_width;  /* 一行的字节数 = 8 * tile_width */

  ptr = u8g2_GetBufferPtr(u8g2);  // 获取缓冲区指针
  ptr += tx*8;                    // 偏移到指定列
  ptr += page_size*ty;            // 偏移到指定行

  while( th > 0 )
  {
    u8x8_DrawTile( u8g2_GetU8x8(u8g2), tx, ty, tw, ptr );  // 发送一行tile数据
    ptr += page_size;   // 移动到下一行
    ty++;
    th--;
  }
}

/**
 * @brief 更新显示器（不发送电子纸刷新消息）
 * @param u8g2  u8g2显示结构体指针
 *
 * 将缓冲区数据发送到显示器，但不发送刷新命令。
 * 适用于非电子纸显示器。
 */
void u8g2_UpdateDisplay(u8g2_t *u8g2)
{
  u8g2_send_buffer(u8g2);
}


/*============================================*/

/**
 * @brief 将缓冲区导出为PBM格式（垂直顶部内存架构）
 * @param u8g2  u8g2显示结构体指针
 * @param out   输出回调函数，用于输出PBM格式的字符串
 *
 * PBM (Portable Bitmap) 是一种简单的黑白图像格式。
 * 此函数适用于大多数显示器（垂直顶部内存架构）。
 */
void u8g2_WriteBufferPBM(u8g2_t *u8g2, void (*out)(const char *s))
{
  u8x8_capture_write_pbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
  u8x8_capture_write_pbm_buffer(u8g2_GetBufferPtr(u8g2), u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), u8x8_capture_get_pixel_1, out);
}

/**
 * @brief 将缓冲区导出为XBM格式（垂直顶部内存架构）
 * @param u8g2  u8g2显示结构体指针
 * @param out   输出回调函数，用于输出XBM格式的字符串
 *
 * XBM 是一种C语言风格的位图格式，常用于嵌入式系统中的图标定义。
 */
void u8g2_WriteBufferXBM(u8g2_t *u8g2, void (*out)(const char *s))
{
  u8x8_capture_write_xbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
  u8x8_capture_write_xbm_buffer(u8g2_GetBufferPtr(u8g2), u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), u8x8_capture_get_pixel_1, out);
}


/**
 * @brief 将缓冲区导出为PBM格式（水平右侧内存架构）
 * @param u8g2  u8g2显示结构体指针
 * @param out   输出回调函数
 *
 * 适用于水平右侧内存架构的显示器，如：SH1122, LD7032, ST7920, ST7986,
 * LC7981, T6963, SED1330, RA8835, MAX7219, LS0等。
 */
void u8g2_WriteBufferPBM2(u8g2_t *u8g2, void (*out)(const char *s))
{
  u8x8_capture_write_pbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
  u8x8_capture_write_pbm_buffer(u8g2_GetBufferPtr(u8g2), u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), u8x8_capture_get_pixel_2, out);
}

/**
 * @brief 将缓冲区导出为XBM格式（水平右侧内存架构）
 * @param u8g2  u8g2显示结构体指针
 * @param out   输出回调函数
 *
 * 适用于水平右侧内存架构的显示器。
 */
void u8g2_WriteBufferXBM2(u8g2_t *u8g2, void (*out)(const char *s))
{
  u8x8_capture_write_xbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
  u8x8_capture_write_xbm_buffer(u8g2_GetBufferPtr(u8g2), u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), u8x8_capture_get_pixel_2, out);
}

