/**
 * @file u8g2_d_memory.c
 * @brief u8g2库的显示缓冲区内存管理文件
 *
 * 本文件负责为u8g2图形库分配显示缓冲区内存。
 * 缓冲区用于存储显示屏的像素数据，采用分页(Page)机制进行管理。
 * 对于128x64的显示屏，需要1024字节的缓冲区(128/8 * 64 = 16*8个tile)。
 *
 * 支持两种内存分配方式：
 * 1. 静态分配：使用固定大小的静态数组（适合嵌入式系统）
 * 2. 动态分配：使用malloc等动态内存分配（需要定义U8G2_USE_DYNAMIC_ALLOC宏）
 *
 * @note 本文件由u8g2项目的codebuild工具自动生成
 */

/* generated code, codebuild, u8g2 project */

#include "u8g2.h"

/**
 * @brief 为16个tile宽、8个page高的显示屏分配全屏缓冲区
 *
 * 该函数为128x64像素的显示屏（如SSD1306）分配1024字节的显示缓冲区。
 * 16个tile宽 = 16 * 8 = 128像素宽
 * 8个page高 = 8 * 8 = 64像素高
 * 缓冲区大小 = 128 * 64 / 8 = 1024字节
 *
 * @param page_cnt [输出] 返回缓冲区包含的page数量，此处为8
 * @return 指向显示缓冲区的指针，若使用动态分配则返回0(NULL)
 *
 * @note "f"后缀表示全屏(Full)缓冲区模式，一次性缓存整个屏幕内容
 * @note 静态缓冲区使用static关键字，函数返回后内存仍然有效
 */
uint8_t *u8g2_m_16_8_f(uint8_t *page_cnt)
{
  #ifdef U8G2_USE_DYNAMIC_ALLOC
  /* 动态内存分配模式：需要外部调用malloc分配内存 */
  *page_cnt = 8;       /* 全屏共8个page (64像素 / 8像素每page) */
  return 0;            /* 返回0，由外部负责分配实际内存 */
  #else
  /* 静态内存分配模式：使用固定大小的静态数组（嵌入式系统推荐方式） */
  static uint8_t buf[1024];  /* 128x64像素 / 8 = 1024字节的显示缓冲区 */
  *page_cnt = 8;             /* 全屏共8个page */
  return buf;                /* 返回静态缓冲区指针 */
  #endif
}

/* end of generated code */
