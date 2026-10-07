/**
 * @file u8g2_font.c
 * @brief u8g2图形库的字体处理核心文件
 *
 * 本文件实现了u8g2库的完整字体渲染系统，包括：
 *
 * 1. 字体数据结构解析：读取字体文件的元数据（字形数量、尺寸、偏移等）
 * 2. 字形数据解码：将压缩的RLE格式字形数据解码为像素
 * 3. 字形渲染：将解码后的字形绘制到显示缓冲区
 * 4. 字符串绘制：支持ASCII和UTF-8编码的字符串渲染
 * 5. 字体旋转：支持0/90/180/270度的文字旋转
 * 6. 字符串宽度计算：计算字符串在屏幕上的像素宽度
 * 7. 字体位置控制：支持基线、顶部、底部、居中等对齐方式
 *
 * u8g2使用自定义的压缩字体格式，具有以下特点：
 * - 使用RLE(Run-Length Encoding)压缩，节省存储空间
 * - 支持比例字体和等宽字体
 * - 支持ASCII和Unicode字符集
 * - 字体数据存储在程序闪存(Flash)中，通过pgm_read接口访问
 *
 * 字体数据结构说明（23字节头部）：
 * - 偏移0-8: 基本参数（字形数量、bbx模式、RLE参数）
 * - 偏移9-12: 最大字符尺寸和偏移
 * - 偏移13-16: 上升/下降参数（用于垂直对齐）
 * - 偏移17-22: ASCII和Unicode字符集的起始位置
 */

/*

  u8g2_font.c

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

#include "u8g2.h"

/* 字体数据结构大小（字节数），没有使用struct或class */
/* 这是新版字体格式的头部大小 */
#define U8G2_FONT_DATA_STRUCT_SIZE 23

/*
  字体数据结构详细说明：

  偏移量	字节数	描述
  0		1		glyph_cnt			字形总数
  1		1		bbx_mode			边界框模式：0=比例字体, 1=等高字体, 2=等宽字体, 3=8像素倍数
  2		1		bits_per_0			背景像素的RLE编码位数
  3		1		bits_per_1			前景像素的RLE编码位数

  4		1		bits_per_char_width		字符宽度的编码位数
  5		1		bits_per_char_height	字符高度的编码位数
  6		1		bits_per_char_x		字符x偏移的编码位数
  7		1		bits_per_char_y		字符y偏移的编码位数
  8		1		bits_per_delta_x		字符间距(delta x)的编码位数

  9		1		max_char_width		最大字符宽度（像素）
  10		1		max_char_height		最大字符高度（像素）
  11		1		x_offset			x方向偏移量
  12		1		y_offset			y方向偏移量（下降部）

  13		1		ascent_A			大写字母A的上升高度
  14		1		descent_g			小写字母g的下降深度
  15		1		ascent_para			左括号(的上升高度
  16		1		descent_para		右括号)的下降深度

  17		1		start_pos_upper_A	大写字母A区域起始位置（高字节）
  18		1		start_pos_upper_A	大写字母A区域起始位置（低字节）

  19		1		start_pos_lower_a	小写字母a区域起始位置（高字节）
  20		1		start_pos_lower_a	小写字母a区域起始位置（低字节）

  21		1		start_pos_unicode	Unicode字符区域起始位置（高字节）
  22		1		start_pos_unicode	Unicode字符区域起始位置（低字节）

  字体构建模式说明：
  - 模式0: 比例字体，最紧凑格式，不同字符高度可能不同
  - 模式1: 等高字体，所有字符使用相同高度
  - 模式2: 等宽字体，所有字符使用相同宽度
  - 模式3: 8像素倍数字体，尺寸为8的整数倍

  字体高度模式：
  - TEXT模式: 仅使用A的上升和g的下降作为参考
  - XTEXT模式: 使用括号的上升/下降扩展参考范围
  - ALL模式: 使用最大字符高度作为参考
*/

/* use case: What is the width and the height of the minimal box into which string s fints? */
void u8g2_font_GetStrSize(const void *font, const char *s, u8g2_uint_t *width, u8g2_uint_t *height);
void u8g2_font_GetStrSizeP(const void *font, const char *s, u8g2_uint_t *width, u8g2_uint_t *height);

/* use case: lower left edge of a minimal box is known, what is the correct x, y position for the string draw procedure */
void u8g2_font_AdjustXYToDraw(const void *font, const char *s, u8g2_uint_t *x, u8g2_uint_t *y);
void u8g2_font_AdjustXYToDrawP(const void *font, const char *s, u8g2_uint_t *x, u8g2_uint_t *y);

/* use case: Baseline origin known, return minimal box */
void u8g2_font_GetStrMinBox(u8g2_t *u8g2, const void *font, const char *s, u8g2_uint_t *x, u8g2_uint_t *y, u8g2_uint_t *width, u8g2_uint_t *height);

/* 过程函数 */

/*========================================================================*/
/* 底层字节和字(word)访问函数 */
/* 这些函数用于从存储在Flash中的字体数据读取字节和16位字 */
/* 使用u8x8_pgm_read确保从程序存储器正确读取数据 */

/**
 * @brief 从字体数据中读取一个字节
 *
 * @param font   字体数据指针（指向Flash中的字体数据）
 * @param offset 从字体数据起始位置的偏移量
 * @return 读取到的字节值
 */
/* 移除了NOINLINE属性，因为这样生成的代码更小，可能也更快 */
//static uint8_t u8g2_font_get_byte(const uint8_t *font, uint8_t offset) U8G2_NOINLINE;
static uint8_t u8g2_font_get_byte(const uint8_t *font, uint8_t offset)
{
  font += offset;                    /* 计算实际地址 */
  return u8x8_pgm_read( font );     /* 从程序存储器读取一个字节 */
}

/**
 * @brief 从字体数据中读取一个16位字（大端序）
 *
 * 字体数据采用大端序(Big-Endian)存储，即高字节在前，低字节在后。
 *
 * @param font   字体数据指针
 * @param offset 从字体数据起始位置的偏移量
 * @return 读取到的16位字值
 */
static uint16_t u8g2_font_get_word(const uint8_t *font, uint8_t offset) U8G2_NOINLINE;
static uint16_t u8g2_font_get_word(const uint8_t *font, uint8_t offset)
{
    uint16_t pos;
    font += offset;
    pos = u8x8_pgm_read( font );    /* 读取高字节 */
    font++;
    pos <<= 8;                       /* 高字节左移8位 */
    pos += u8x8_pgm_read( font);    /* 读取低字节并合并 */
    return pos;
}

/*========================================================================*/
/* 字体信息读取函数 */
/* 从字体数据头部读取所有元数据到font_info结构体中 */

/**
 * @brief 读取字体的元数据信息
 *
 * 从字体数据的头部（23字节）读取所有参数到u8g2_font_info_t结构体中。
 * 这些信息包括字形数量、编码参数、尺寸信息和字符集起始位置等。
 *
 * @param font_info [输出] 字体信息结构体指针，用于存储读取的结果
 * @param font      字体数据指针（指向Flash中的字体数据头部）
 */
void u8g2_read_font_info(u8g2_font_info_t *font_info, const uint8_t *font)
{
  /* 读取偏移0-3：基本参数 */
  font_info->glyph_cnt = u8g2_font_get_byte(font, 0);      /* 字形总数 */
  font_info->bbx_mode = u8g2_font_get_byte(font, 1);       /* 边界框模式 */
  font_info->bits_per_0 = u8g2_font_get_byte(font, 2);     /* 背景像素RLE编码位数 */
  font_info->bits_per_1 = u8g2_font_get_byte(font, 3);     /* 前景像素RLE编码位数 */

  /* 读取偏移4-8：RLE编码参数 */
  font_info->bits_per_char_width = u8g2_font_get_byte(font, 4);   /* 字符宽度编码位数 */
  font_info->bits_per_char_height = u8g2_font_get_byte(font, 5);  /* 字符高度编码位数 */
  font_info->bits_per_char_x = u8g2_font_get_byte(font, 6);       /* 字符x偏移编码位数 */
  font_info->bits_per_char_y = u8g2_font_get_byte(font, 7);       /* 字符y偏移编码位数 */
  font_info->bits_per_delta_x = u8g2_font_get_byte(font, 8);      /* 字符间距编码位数 */

  /* 读取偏移9-12：尺寸和偏移参数 */
  font_info->max_char_width = u8g2_font_get_byte(font, 9);    /* 最大字符宽度 */
  font_info->max_char_height = u8g2_font_get_byte(font, 10);  /* 最大字符高度 */
  font_info->x_offset = u8g2_font_get_byte(font, 11);         /* x方向偏移 */
  font_info->y_offset = u8g2_font_get_byte(font, 12);         /* y方向偏移（下降部） */

  /* 读取偏移13-16：上升/下降参数（用于垂直对齐计算） */
  font_info->ascent_A = u8g2_font_get_byte(font, 13);       /* 大写A的上升高度 */
  font_info->descent_g = u8g2_font_get_byte(font, 14);      /* 小写g的下降深度 */
  font_info->ascent_para = u8g2_font_get_byte(font, 15);    /* 左括号(的上升高度 */
  font_info->descent_para = u8g2_font_get_byte(font, 16);   /* 右括号)的下降深度 */

  /* 读取偏移17-20：ASCII字符集起始位置（16位字，大端序） */
  font_info->start_pos_upper_A = u8g2_font_get_word(font, 17);  /* 大写字母A区域起始位置 */
  font_info->start_pos_lower_a = u8g2_font_get_word(font, 19);  /* 小写字母a区域起始位置 */

  /* 读取偏移21-22：Unicode字符集起始位置 */
#ifdef U8G2_WITH_UNICODE
  font_info->start_pos_unicode = u8g2_font_get_word(font, 21);  /* Unicode区域起始位置 */
#endif

#ifdef U8G2_WITH_UNICODE
  font_info->start_pos_unicode = u8g2_font_get_word(font, 21); 
#endif
}



/**
 * @brief 计算字体数据的总长度（字节数）
 *
 * 遍历字体数据中的所有字形，计算整个字体数据的总字节数。
 * 主要用于生成字体数据的可视化图表（Google Wiki）。
 *
 * @param font_arg 字体数据指针
 * @return 字体数据的总字节数
 *
 * @note 此函数主要用于调试和文档生成，正常应用中较少使用
 */
size_t u8g2_GetFontSize(const uint8_t *font_arg)
{
  uint16_t e;
  const uint8_t *font = font_arg;
  font += U8G2_FONT_DATA_STRUCT_SIZE;  /* 跳过23字节的头部 */

  /* 遍历ASCII字形数据，直到遇到结束标记（字形大小为0） */
  for(;;)
  {
    if ( u8x8_pgm_read( font + 1 ) == 0 )  /* 检查字形大小是否为0（结束标记） */
      break;
    font += u8x8_pgm_read( font + 1 );     /* 跳过当前字形数据 */
  }

  /* 继续处理Unicode部分 */
  font += 2;  /* 跳过Unicode区域的起始标记 */

  /* 跳过Unicode查找表 */
  font += u8g2_font_get_word(font, 0);

  /* 遍历Unicode字形数据，直到遇到结束标记（编码为0） */
  for(;;)
  {
    e = u8x8_pgm_read( font );      /* 读取Unicode编码高字节 */
    e <<= 8;
    e |= u8x8_pgm_read( font + 1 ); /* 读取Unicode编码低字节 */
    if ( e == 0 )                    /* 编码为0表示结束 */
      break;
    font += u8x8_pgm_read( font + 2 );  /* 跳过当前字形数据 */
  }

  return (font - font_arg) + 2;  /* 返回总字节数（+2包含结束标记） */
}

/*========================================================================*/
/* u8g2接口层：字体属性访问函数 */
/* 这些函数提供了访问字体各种属性的公共接口 */

/**
 * @brief 获取字体的最大字符宽度
 *
 * @param u8g2 u8g2设备结构体指针
 * @return 最大字符宽度（像素）
 */
uint8_t u8g2_GetFontBBXWidth(u8g2_t *u8g2)
{
  return u8g2->font_info.max_char_width;  /* 返回字体中最大字符的宽度 */
}

/**
 * @brief 获取字体的最大字符高度
 *
 * @param u8g2 u8g2设备结构体指针
 * @return 最大字符高度（像素）
 */
uint8_t u8g2_GetFontBBXHeight(u8g2_t *u8g2)
{
  return u8g2->font_info.max_char_height;  /* 返回字体中最大字符的高度 */
}

/**
 * @brief 获取字体的x方向偏移量
 *
 * @param u8g2 u8g2设备结构体指针
 * @return x方向偏移量（有符号）
 */
int8_t u8g2_GetFontBBXOffX(u8g2_t *u8g2) U8G2_NOINLINE;
int8_t u8g2_GetFontBBXOffX(u8g2_t *u8g2)
{
  return u8g2->font_info.x_offset;  /* 返回x方向偏移量 */
}

/**
 * @brief 获取字体的y方向偏移量
 *
 * @param u8g2 u8g2设备结构体指针
 * @return y方向偏移量（有符号，通常为负值表示下降部）
 */
int8_t u8g2_GetFontBBXOffY(u8g2_t *u8g2) U8G2_NOINLINE;
int8_t u8g2_GetFontBBXOffY(u8g2_t *u8g2)
{
  return u8g2->font_info.y_offset;  /* 返回y方向偏移量 */
}

/**
 * @brief 获取大写字母A的高度（上升高度）
 *
 * 大写A的高度常用于计算字体的上升高度(ascent)，
 * 是垂直对齐的重要参考值。
 *
 * @param u8g2 u8g2设备结构体指针
 * @return 大写A的高度（像素）
 */
uint8_t u8g2_GetFontCapitalAHeight(u8g2_t *u8g2) U8G2_NOINLINE;
uint8_t u8g2_GetFontCapitalAHeight(u8g2_t *u8g2)
{
  return u8g2->font_info.ascent_A;  /* 返回大写A的上升高度 */
}

/*========================================================================*/
/* 字形解码处理函数 */
/* 这些函数负责从压缩的字体数据中解码出字形的像素信息 */

/**
 * @brief 从字形数据流中读取指定数量的无符号位
 *
 * 字形数据使用变长位编码，此函数从数据流中读取指定位数的无符号值。
 * 使用位流解码方式，可以高效地从压缩数据中提取变长编码的值。
 *
 * @param f   字形解码状态结构体指针，包含当前读取位置和位偏移
 * @param cnt 要读取的位数（1-8）
 * @return 读取到的无符号值
 *
 * @note 该函数会自动处理跨字节边界的位读取
 */
/* 优化版本 */
uint8_t u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t *f, uint8_t cnt)
{
  uint8_t val;
  uint8_t bit_pos = f->decode_bit_pos;      /* 当前字节内的位偏移 */
  uint8_t bit_pos_plus_cnt;

  val = u8x8_pgm_read( f->decode_ptr );     /* 从Flash读取当前字节 */

  val >>= bit_pos;                           /* 右移到对齐位置 */
  bit_pos_plus_cnt = bit_pos;
  bit_pos_plus_cnt += cnt;                   /* 计算读取后的位偏移 */
  if ( bit_pos_plus_cnt >= 8 )               /* 检查是否跨越字节边界 */
  {
    uint8_t s = 8;
    s -= bit_pos;                            /* 计算当前字节剩余的位数 */
    f->decode_ptr++;                         /* 移动到下一个字节 */
    val |= u8x8_pgm_read( f->decode_ptr ) << (s);  /* 读取下一个字节并合并 */
    bit_pos_plus_cnt -= 8;                   /* 调整位偏移到新字节中 */
  }
  val &= (1U<<cnt)-1;                       /* 掩码提取所需的位数 */

  f->decode_bit_pos = bit_pos_plus_cnt;      /* 更新位偏移状态 */
  return val;
}


/*
  有符号位解码说明：
  2位编码 --> cnt = 2, 范围: -2,-1,0,1
  3位编码 --> cnt = 3, 范围: -4,-3,-2,-1,0,1,2,3

  编码方式：使用偏移二进制编码
  如果 x < 0: r = bits(x-1)+1
  如果 x >= 0: r = bits(x)+1
*/
/**
 * @brief 从字形数据流中读取指定数量的有符号位
 *
 * 使用偏移二进制编码方式将无符号值转换为有符号值。
 * 例如：2位编码的值0,1,2,3被转换为-2,-1,0,1
 *
 * @param f   字形解码状态结构体指针
 * @param cnt 要读取的位数（1-8）
 * @return 读取到的有符号值
 */
/* 优化版本 */
int8_t u8g2_font_decode_get_signed_bits(u8g2_font_decode_t *f, uint8_t cnt)
{
  int8_t v, d;
  v = (int8_t)u8g2_font_decode_get_unsigned_bits(f, cnt);  /* 先读取无符号值 */
  d = 1;
  cnt--;
  d <<= cnt;       /* 计算偏移量：2^(cnt-1) */
  v -= d;           /* 减去偏移量，将无符号值转换为有符号值 */
  return v;
}


/* 字体旋转支持函数 */
/* 这些函数根据旋转方向将逻辑坐标转换为物理坐标 */

#ifdef U8G2_WITH_FONT_ROTATION
/**
 * @brief 根据旋转方向计算y坐标分量
 *
 * 将相对于字形原点的(x,y)偏移按照旋转方向转换为屏幕y坐标。
 *
 * @param dy  目标位置的y坐标基准值
 * @param x   字形局部x坐标偏移
 * @param y   字形局部y坐标偏移
 * @param dir 旋转方向：0=正常, 1=90度, 2=180度, 3=270度
 * @return 变换后的y坐标
 */
u8g2_uint_t u8g2_add_vector_y(u8g2_uint_t dy, int8_t x, int8_t y, uint8_t dir)
{
  switch(dir)
  {
    case 0:      /* 0度：y坐标直接加上y偏移 */
      dy += y;
      break;
    case 1:      /* 90度：y坐标加上x偏移（坐标轴旋转） */
      dy += x;
      break;
    case 2:      /* 180度：y坐标减去y偏移（上下翻转） */
      dy -= y;
      break;
    default:     /* 270度：y坐标减去x偏移 */
      dy -= x;
      break;
  }
  return dy;
}

/**
 * @brief 根据旋转方向计算x坐标分量
 *
 * @param dx  目标位置的x坐标基准值
 * @param x   字形局部x坐标偏移
 * @param y   字形局部y坐标偏移
 * @param dir 旋转方向：0=正常, 1=90度, 2=180度, 3=270度
 * @return 变换后的x坐标
 */
u8g2_uint_t u8g2_add_vector_x(u8g2_uint_t dx, int8_t x, int8_t y, uint8_t dir)
{
  switch(dir)
  {
    case 0:      /* 0度：x坐标直接加上x偏移 */
      dx += x;
      break;
    case 1:      /* 90度：x坐标减去y偏移（坐标轴旋转） */
      dx -= y;
      break;
    case 2:      /* 180度：x坐标减去x偏移（左右翻转） */
      dx -= x;
      break;
    default:     /* 270度：x坐标加上y偏移 */
      dx += y;
      break;
  }
  return dx;
}

/*
// 备用的组合函数，但在AVR上需要额外50字节，因此不使用
void u8g2_add_vector(u8g2_uint_t *xp, u8g2_uint_t *yp, int8_t x, int8_t y, uint8_t dir)
{
  u8g2_uint_t x_ = *xp;
  u8g2_uint_t y_ = *yp;
  switch(dir)
  {
    case 0:
      y_ += y;
      x_ += x;
      break;
    case 1:
      y_ += x;
      x_ -= y;
      break;
    case 2:
      y_ -= y;
      x_ -= x;
      break;
    default:
      y_ -= x;
      x_ += y;
      break;
  }
  *xp = x_;
  *yp = y_;
}
*/
#endif


/**
 * @brief 绘制字形的RLE编码线段
 *
 * 将字形中的一个RLE(Run-Length Encoding)编码段解码并绘制到屏幕上。
 * RLE编码是一种压缩方式，用"重复次数+值"来表示连续的相同像素。
 *
 * 该函数处理字形中的一段连续像素，可能是前景色（字符笔画）或背景色。
 * 当线段跨越字形边界时，会自动换行到下一行继续绘制。
 *
 * @param u8g2           u8g2设备结构体指针
 * @param len            要绘制的像素长度
 * @param is_foreground  是否为前景色：1=前景色（绘制），0=背景色（透明模式下不绘制）
 *
 * @note 该函数被u8g2_font_decode_glyph()调用
 * @note 支持字体旋转（通过U8G2_WITH_FONT_ROTATION宏控制）
 * @note 透明模式下，背景色的像素不会被绘制
 */
/* 优化版本 */
void u8g2_font_decode_len(u8g2_t *u8g2, uint8_t len, uint8_t is_foreground)
{
  uint8_t cnt;	/* total number of remaining pixels, which have to be drawn */
  uint8_t rem; 	/* remaining pixel to the right edge of the glyph */
  uint8_t current;	/* number of pixels, which need to be drawn for the draw procedure */
    /* current is either equal to cnt or equal to rem */
  
  /* local coordinates of the glyph */
  uint8_t lx,ly;
  
  /* target position on the screen */
  u8g2_uint_t x, y;
  
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  
  cnt = len;
  
  /* get the local position */
  lx = decode->x;
  ly = decode->y;
  
  for(;;)
  {
    /* calculate the number of pixel to the right edge of the glyph */
    rem = decode->glyph_width;
    rem -= lx;
    
    /* calculate how many pixel to draw. This is either to the right edge */
    /* or lesser, if not enough pixel are left */
    current = rem;
    if ( cnt < rem )
      current = cnt;
    
    
    /* now draw the line, but apply the rotation around the glyph target position */
    //u8g2_font_decode_draw_pixel(u8g2, lx,ly,current, is_foreground);

    /* get target position */
    x = decode->target_x;
    y = decode->target_y;

    /* apply rotation */
#ifdef U8G2_WITH_FONT_ROTATION
    
    x = u8g2_add_vector_x(x, lx, ly, decode->dir);
    y = u8g2_add_vector_y(y, lx, ly, decode->dir);
    
    //u8g2_add_vector(&x, &y, lx, ly, decode->dir);
    
#else
    x += lx;
    y += ly;
#endif
    
    /* draw foreground and background (if required) */
    if ( is_foreground )
    {
      u8g2->draw_color = decode->fg_color;			/* draw_color will be restored later */
      u8g2_DrawHVLine(u8g2, 
	x, 
	y, 
	current, 
#ifdef U8G2_WITH_FONT_ROTATION
	/* dir */ decode->dir
#else
	0
#endif
      );
    }
    else if ( decode->is_transparent == 0 )    
    {
      u8g2->draw_color = decode->bg_color;			/* draw_color will be restored later */
      u8g2_DrawHVLine(u8g2, 
	x, 
	y, 
	current, 
#ifdef U8G2_WITH_FONT_ROTATION
	/* dir */ decode->dir
#else
	0
#endif
      );   
    }
    
    /* check, whether the end of the run length code has been reached */
    if ( cnt < rem )
      break;
    cnt -= rem;
    lx = 0;
    ly++;
  }
  lx += cnt;
  
  decode->x = lx;
  decode->y = ly;  
}


/**
 * @brief 绘制2倍放大字形的RLE编码线段
 *
 * 与u8g2_font_decode_len类似，但将字形放大2倍绘制。
 * 每个像素会被绘制为2x2的像素块。
 *
 * @param u8g2           u8g2设备结构体指针
 * @param len            要绘制的像素长度
 * @param is_foreground  是否为前景色：1=前景色（绘制），0=背景色（透明模式下不绘制）
 */
void u8g2_font_2x_decode_len(u8g2_t *u8g2, uint8_t len, uint8_t is_foreground)
{
  uint8_t cnt;	/* total number of remaining pixels, which have to be drawn */
  uint8_t rem; 	/* remaining pixel to the right edge of the glyph */
  uint8_t current;	/* number of pixels, which need to be drawn for the draw procedure */
    /* current is either equal to cnt or equal to rem */
  
  /* local coordinates of the glyph */
  uint8_t lx,ly;
  
  /* target position on the screen */
  u8g2_uint_t x, y;
  
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  
  cnt = len;
  
  /* get the local position */
  lx = decode->x;
  ly = decode->y;
  
  for(;;)
  {
    /* calculate the number of pixel to the right edge of the glyph */
    rem = decode->glyph_width;
    rem -= lx;
    
    /* calculate how many pixel to draw. This is either to the right edge */
    /* or lesser, if not enough pixel are left */
    current = rem;
    if ( cnt < rem )
      current = cnt;
    
    
    /* now draw the line, but apply the rotation around the glyph target position */
    //u8g2_font_decode_draw_pixel(u8g2, lx,ly,current, is_foreground);

    /* get target position */
    x = decode->target_x;
    y = decode->target_y;

    x += lx*2;
    y += ly*2;
    
    /* draw foreground and background (if required) */
    if ( is_foreground )
    {
      u8g2->draw_color = decode->fg_color;			/* draw_color will be restored later */
      u8g2_DrawHVLine(u8g2, 
	x, 
	y, 
	current*2, 
	0
      );
      u8g2_DrawHVLine(u8g2, 
	x, 
	y+1, 
	current*2, 
	0
      );
    }
    else if ( decode->is_transparent == 0 )    
    {
      u8g2->draw_color = decode->bg_color;			/* draw_color will be restored later */
      u8g2_DrawHVLine(u8g2, 
	x, 
	y, 
	current*2, 
	0
      );   
      u8g2_DrawHVLine(u8g2, 
	x, 
	y+1, 
	current*2, 
	0
      );   
    }
    
    /* check, whether the end of the run length code has been reached */
    if ( cnt < rem )
      break;
    cnt -= rem;
    lx = 0;
    ly++;
  }
  lx += cnt;
  
  decode->x = lx;
  decode->y = ly;
  
}


/**
 * @brief 初始化字形解码器
 *
 * 设置字形解码器的初始状态，包括：
 * - 设置数据指针和位偏移
 * - 从字形数据中读取宽度和高度
 * - 设置前景色和背景色
 *
 * @param u8g2       u8g2设备结构体指针
 * @param glyph_data 字形数据指针（指向Flash中的压缩字形数据）
 */
static void u8g2_font_setup_decode(u8g2_t *u8g2, const uint8_t *glyph_data)
{
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
  decode->decode_ptr = glyph_data;     /* 设置数据读取指针 */
  decode->decode_bit_pos = 0;          /* 位偏移初始化为0 */

  /* 2015年11月8日：编码和大小字段的跳过已在字形数据搜索过程中完成 */
  /*
  decode->decode_ptr += 1;
  decode->decode_ptr += 1;
  */

  /* 从字形数据中读取宽度和高度（使用变长编码） */
  decode->glyph_width = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
  decode->glyph_height = u8g2_font_decode_get_unsigned_bits(decode,u8g2->font_info.bits_per_char_height);

  /* 设置前景色和背景色 */
  decode->fg_color = u8g2->draw_color;                     /* 前景色使用当前绘图颜色 */
  decode->bg_color = (decode->fg_color == 0 ? 1 : 0);      /* 背景色为前景色的反色 */
}


/**
 * @brief 解码并绘制一个字形
 *
 * 这是字形渲染的核心函数，负责：
 * 1. 初始化解码器并读取字形的基本参数（x/y偏移、间距）
 * 2. 计算字形在屏幕上的实际绘制位置
 * 3. 执行相交检测（如果支持），跳过屏幕外的字形
 * 4. 循环解码RLE编码的像素数据并绘制
 *
 * @param u8g2       u8g2设备结构体指针
 * @param glyph_data 字形数据指针（指向Flash中的压缩字形数据）
 * @return 字形的水平推进距离(delta x)，即下一个字符的x偏移量
 *
 * @note target_x和target_y必须在调用前设置为字形的绘制位置
 * @note is_transparent标志控制是否绘制背景像素
 */
/* 优化版本 */
int8_t u8g2_font_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data)
{
  uint8_t a, b;
  int8_t x, y;
  int8_t d;
  int8_t h;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
    
  u8g2_font_setup_decode(u8g2, glyph_data);     /* set values in u8g2->font_decode data structure */
  h = u8g2->font_decode.glyph_height;
  
  x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
  y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
  d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);
  
  if ( decode->glyph_width > 0 )
  {
#ifdef U8G2_WITH_FONT_ROTATION
    decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h+y), decode->dir);
    decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h+y), decode->dir);
    
    //u8g2_add_vector(&(decode->target_x), &(decode->target_y), x, -(h+y), decode->dir);

#else
    decode->target_x += x;
    decode->target_y -= h+y;
#endif
    //u8g2_add_vector(&(decode->target_x), &(decode->target_y), x, -(h+y), decode->dir);

#ifdef U8G2_WITH_INTERSECTION
    {
      u8g2_uint_t x0, x1, y0, y1;
      x0 = decode->target_x;
      y0 = decode->target_y;
      x1 = x0;
      y1 = y0;
      
#ifdef U8G2_WITH_FONT_ROTATION
      switch(decode->dir)
      {
	case 0:
	    x1 += decode->glyph_width;
	    y1 += h;
	    break;
	case 1:
	    x0 -= h;
	    x0++;	/* shift down, because of assymetric boundaries for the interseciton test */
	    x1++;
	    y1 += decode->glyph_width;
	    break;
	case 2:
	    x0 -= decode->glyph_width;
	    x0++;	/* shift down, because of assymetric boundaries for the interseciton test */
	    x1++;
	    y0 -= h;
	    y0++;	/* shift down, because of assymetric boundaries for the interseciton test */
	    y1++;
	    break;	  
	case 3:
	    x1 += h;
	    y0 -= decode->glyph_width;
	    y0++;	/* shift down, because of assymetric boundaries for the interseciton test */
	    y1++;
	    break;	  
      }
#else /* U8G2_WITH_FONT_ROTATION */
      x1 += decode->glyph_width;
      y1 += h;      
#endif
      
      if ( u8g2_IsIntersection(u8g2, x0, y0, x1, y1) == 0 ) 
	return d;
    }
#endif /* U8G2_WITH_INTERSECTION */
   
    /* reset local x/y position */
    decode->x = 0;
    decode->y = 0;
    
    /* decode glyph */
    for(;;)
    {
      a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
      b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
      do
      {
	u8g2_font_decode_len(u8g2, a, 0);
	u8g2_font_decode_len(u8g2, b, 1);
      } while( u8g2_font_decode_get_unsigned_bits(decode, 1) != 0 );

      if ( decode->y >= h )
	break;
    }
    
    /* restore the u8g2 draw color, because this is modified by the decode algo */
    u8g2->draw_color = decode->fg_color;
  }
  return d;
}


/**
 * @brief 解码并绘制一个2倍放大的字形
 *
 * 与u8g2_font_decode_glyph功能相同，但字形被放大2倍显示。
 * 适用于需要大字体显示但字体库中没有对应大字体的情况。
 *
 * @param u8g2       u8g2设备结构体指针
 * @param glyph_data 字形数据指针（指向Flash中的压缩字形数据）
 * @return 字形的水平推进距离(delta x)的2倍值
 */
int8_t u8g2_font_2x_decode_glyph(u8g2_t *u8g2, const uint8_t *glyph_data)
{
  uint8_t a, b;
  int8_t x, y;
  int8_t d;
  int8_t h;
  u8g2_font_decode_t *decode = &(u8g2->font_decode);
    
  u8g2_font_setup_decode(u8g2, glyph_data);     /* set values in u8g2->font_decode data structure */
  h = u8g2->font_decode.glyph_height;
  
  x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
  y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
  d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);
  
  if ( decode->glyph_width > 0 )
  {
    decode->target_x += x;
    decode->target_y -= 2*h+y;

#ifdef U8G2_WITH_INTERSECTION
    {
      u8g2_uint_t x0, x1, y0, y1;
      x0 = decode->target_x;
      y0 = decode->target_y;
      x1 = x0;
      y1 = y0;
      
      x1 += 2*decode->glyph_width;
      y1 += 2*h;      
      
      if ( u8g2_IsIntersection(u8g2, x0, y0, x1, y1) == 0 ) 
	return d;
    }
#endif /* U8G2_WITH_INTERSECTION */
   
    /* reset local x/y position */
    decode->x = 0;
    decode->y = 0;
    
    /* decode glyph */
    for(;;)
    {
      a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
      b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
      do
      {
	u8g2_font_2x_decode_len(u8g2, a, 0);
	u8g2_font_2x_decode_len(u8g2, b, 1);
      } while( u8g2_font_decode_get_unsigned_bits(decode, 1) != 0 );

      if ( decode->y >= h )
	break;
    }
    
    /* restore the u8g2 draw color, because this is modified by the decode algo */
    u8g2->draw_color = decode->fg_color;
  }
  return d*2;
}

/**
 * @brief 根据字符编码查找字形数据
 *
 * 在字体数据中搜索指定字符编码对应的字形数据。
 * 支持ASCII（0-255）和Unicode（>255）两种编码范围。
 *
 * 搜索策略优化：
 * - ASCII字符：根据编码范围跳转到对应的起始位置（大写A或小写a区域）
 * - Unicode字符：先通过查找表定位到大致范围，再顺序搜索
 *
 * @param u8g2    u8g2设备结构体指针
 * @param encoding 字符编码（ASCII或Unicode）
 * @return 字形数据指针，如果字体中不包含该字符则返回NULL
 */
const uint8_t *u8g2_font_get_glyph_data(u8g2_t *u8g2, uint16_t encoding)
{
  const uint8_t *font = u8g2->font;
  font += U8G2_FONT_DATA_STRUCT_SIZE;  /* 跳过23字节的字体头部 */

  /* ASCII字符处理（编码 <= 255） */
  if ( encoding <= 255 )
  {
    /* 根据编码范围跳转到对应的起始位置，优化搜索速度 */
    if ( encoding >= 'a' )
    {
      font += u8g2->font_info.start_pos_lower_a;  /* 跳转到小写字母区域 */
    }
    else if ( encoding >= 'A' )
    {
      font += u8g2->font_info.start_pos_upper_A;  /* 跳转到大写字母区域 */
    }

    /* 顺序搜索字形数据 */
    for(;;)
    {
      if ( u8x8_pgm_read( font + 1 ) == 0 )  /* 字形大小为0表示区域结束 */
	break;
      if ( u8x8_pgm_read( font ) == encoding )  /* 找到匹配的编码 */
      {
	return font+2;  /* 返回字形数据指针（跳过编码和大小字段） */
      }
      font += u8x8_pgm_read( font + 1 );  /* 跳转到下一个字形 */
    }
  }
#ifdef U8G2_WITH_UNICODE
  /* Unicode字符处理（编码 > 255） */
  else
  {
    uint16_t e;
    const uint8_t *unicode_lookup_table;

    font += u8g2->font_info.start_pos_unicode;  /* 跳转到Unicode区域 */
    unicode_lookup_table = font;

    /* 使用Unicode查找表快速定位到大致范围（issue 596优化） */
    do
    {
      font += u8g2_font_get_word(unicode_lookup_table, 0);  /* 跳转到该范围的起始位置 */
      e = u8g2_font_get_word(unicode_lookup_table, 2);      /* 读取该范围的结束编码 */
      unicode_lookup_table+=4;                               /* 移动到下一个查找表项 */
    } while( e < encoding );                                 /* 直到找到包含目标编码的范围 */

    /* 在找到的范围内顺序搜索 */
    for(;;)
    {
      e = u8x8_pgm_read( font );       /* 读取Unicode编码高字节 */
      e <<= 8;
      e |= u8x8_pgm_read( font + 1 );  /* 读取Unicode编码低字节 */

      if ( e == 0 )                     /* 编码为0表示Unicode区域结束 */
	break;

      if ( e == encoding )              /* 找到匹配的编码 */
      {
	return font+3;  /* 返回字形数据指针（跳过2字节编码和1字节大小） */
      }
      font += u8x8_pgm_read( font + 2 );  /* 跳转到下一个字形 */
    }
  }
#endif

  return NULL;  /* 未找到匹配的字形，返回NULL */
}

/**
 * @brief 绘制单个字形（内部函数）
 *
 * 查找指定编码的字形数据并在指定位置绘制。
 * 该函数是u8g2_DrawGlyph的内部实现。
 *
 * @param u8g2    u8g2设备结构体指针
 * @param x       字形绘制起始x坐标
 * @param y       字形绘制起始y坐标（基线位置）
 * @param encoding 字符编码（ASCII或Unicode）
 * @return 字形的水平推进距离，用于确定下一个字符的位置
 */
static u8g2_uint_t u8g2_font_draw_glyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding)
{
  u8g2_uint_t dx = 0;
  u8g2->font_decode.target_x = x;
  u8g2->font_decode.target_y = y;
  //u8g2->font_decode.is_transparent = is_transparent; this is already set
  //u8g2->font_decode.dir = dir;
  const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
  if ( glyph_data != NULL )
  {
    dx = u8g2_font_decode_glyph(u8g2, glyph_data);
  }
  return dx;
}

/**
 * @brief 绘制单个2倍放大的字形（内部函数）
 *
 * 查找指定编码的字形数据并在指定位置以2倍大小绘制。
 *
 * @param u8g2    u8g2设备结构体指针
 * @param x       字形绘制起始x坐标
 * @param y       字形绘制起始y坐标（基线位置）
 * @param encoding 字符编码（ASCII或Unicode）
 * @return 字形的水平推进距离的2倍值
 */
static u8g2_uint_t u8g2_font_2x_draw_glyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding)
{
  u8g2_uint_t dx = 0;
  u8g2->font_decode.target_x = x;
  u8g2->font_decode.target_y = y;
  const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
  if ( glyph_data != NULL )
  {
    dx = u8g2_font_2x_decode_glyph(u8g2, glyph_data);
  }
  return dx;
}



/**
 * @brief 检查字体中是否包含指定编码的字形
 *
 * 用于在绘制前验证字符是否存在于当前字体中。
 * 常用于实现字体回退机制或过滤不支持的字符。
 *
 * @param u8g2              u8g2设备结构体指针
 * @param requested_encoding 要检查的字符编码
 * @return 1 表示字形存在，0 表示字形不存在
 */
uint8_t u8g2_IsGlyph(u8g2_t *u8g2, uint16_t requested_encoding)
{
  /* updated to new code */
  if ( u8g2_font_get_glyph_data(u8g2, requested_encoding) != NULL )
    return 1;
  return 0;
}

/**
 * @brief 获取指定字符的水平推进宽度
 *
 * 计算字符绘制后下一个字符应该移动的水平距离。
 * 这个宽度包括字符本身的宽度和字符间距。
 *
 * @param u8g2              u8g2设备结构体指针
 * @param requested_encoding 字符编码
 * @return 字符的水平推进距离（像素），如果字形不存在则返回0
 *
 * @note 副作用：会更新u8g2->font_decode和u8g2->glyph_x_offset
 */
int8_t u8g2_GetGlyphWidth(u8g2_t *u8g2, uint16_t requested_encoding)
{
  const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
  if ( glyph_data == NULL )
    return 0; 
  
  u8g2_font_setup_decode(u8g2, glyph_data);
  u8g2->glyph_x_offset = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
  u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);
  
  /* glyph width is here: u8g2->font_decode.glyph_width */

  return u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}


/**
 * @brief 设置字体绘制模式
 *
 * 控制字体背景的绘制方式：
 * - U8G2_FONT_MODE_TRANSPARENT (1): 透明模式，不绘制字符背景
 * - U8G2_FONT_MODE_SOLID (0): 实心模式，绘制字符背景（使用背景色）
 * - U8G2_FONT_MODE_NONE (2): 无模式，不绘制任何内容
 *
 * @param u8g2          u8g2设备结构体指针
 * @param is_transparent 字体模式值
 */
void u8g2_SetFontMode(u8g2_t *u8g2, uint8_t is_transparent)
{
  u8g2->font_decode.is_transparent = is_transparent;		// new font procedures
}

/**
 * @brief 在指定位置绘制单个字符
 *
 * 这是绘制单个字符的公共API函数。会根据当前字体位置设置（基线、顶部、底部、居中）
 * 自动调整y坐标，然后调用内部函数绘制字形。
 *
 * @param u8g2    u8g2设备结构体指针
 * @param x       字符绘制的x坐标
 * @param y       字符绘制的y坐标（根据字体位置模式解释）
 * @param encoding 字符编码（ASCII或Unicode）
 * @return 字符的水平推进距离，可用于定位下一个字符
 */
u8g2_uint_t u8g2_DrawGlyph(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding)
{
#ifdef U8G2_WITH_FONT_ROTATION
  switch(u8g2->font_decode.dir)
  {
    case 0:
      y += u8g2->font_calc_vref(u8g2);
      break;
    case 1:
      x -= u8g2->font_calc_vref(u8g2);
      break;
    case 2:
      y -= u8g2->font_calc_vref(u8g2);
      break;
    case 3:
      x += u8g2->font_calc_vref(u8g2);
      break;
  }
#else
  y += u8g2->font_calc_vref(u8g2);
#endif
  return u8g2_font_draw_glyph(u8g2, x, y, encoding);
}

/**
 * @brief 在指定位置绘制2倍放大的单个字符
 *
 * 与u8g2_DrawGlyph功能相同，但字符被放大2倍显示。
 * 适用于需要临时使用大字体而不想切换字体的场景。
 *
 * @param u8g2    u8g2设备结构体指针
 * @param x       字符绘制的x坐标
 * @param y       字符绘制的y坐标
 * @param encoding 字符编码
 * @return 字符的水平推进距离（2倍值）
 */
u8g2_uint_t u8g2_DrawGlyphX2(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding)
{
  y += 2*u8g2->font_calc_vref(u8g2);
  return u8g2_font_2x_draw_glyph(u8g2, x, y, encoding);
}

/**
 * @brief 绘制字符串（内部函数，支持旋转）
 *
 * 逐字符解码并绘制字符串，支持UTF-8编码。
 * 根据当前字体旋转方向自动调整字符位置。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param x    字符串起始x坐标
 * @param y    字符串起始y坐标
 * @param str  要绘制的字符串（UTF-8编码）
 * @return 字符串的总水平推进距离（像素宽度）
 */
static u8g2_uint_t u8g2_draw_string(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str) U8G2_NOINLINE;
static u8g2_uint_t u8g2_draw_string(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str)
{
  uint16_t e;
  u8g2_uint_t delta, sum;
  u8x8_utf8_init(u8g2_GetU8x8(u8g2));
  sum = 0;
  for(;;)
  {
    e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      delta = u8g2_DrawGlyph(u8g2, x, y, e);
    
#ifdef U8G2_WITH_FONT_ROTATION
      switch(u8g2->font_decode.dir)
      {
	case 0:
	  x += delta;
	  break;
	case 1:
	  y += delta;
	  break;
	case 2:
	  x -= delta;
	  break;
	case 3:
	  y -= delta;
	  break;
      }
      
      /*
      // requires 10 bytes more on avr
      x = u8g2_add_vector_x(x, delta, 0, u8g2->font_decode.dir);
      y = u8g2_add_vector_y(y, delta, 0, u8g2->font_decode.dir);
      */

#else
      x += delta;
#endif

      sum += delta;    
    }
  }
  return sum;
}

/**
 * @brief 绘制2倍放大的字符串（内部函数）
 *
 * 与u8g2_draw_string功能相同，但所有字符放大2倍显示。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param x    字符串起始x坐标
 * @param y    字符串起始y坐标
 * @param str  要绘制的字符串（UTF-8编码）
 * @return 字符串的总水平推进距离（2倍值）
 */
static u8g2_uint_t u8g2_draw_string_2x(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str) U8G2_NOINLINE;
static u8g2_uint_t u8g2_draw_string_2x(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str)
{
  uint16_t e;
  u8g2_uint_t delta, sum;
  u8x8_utf8_init(u8g2_GetU8x8(u8g2));
  sum = 0;
  for(;;)
  {
    e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      delta = u8g2_DrawGlyphX2(u8g2, x, y, e);
      x += delta;
      sum += delta;    
    }
  }
  return sum;
}

/**
 * @brief 绘制ASCII字符串
 *
 * 在指定位置绘制ASCII编码的字符串。这是最常用的字符串绘制函数。
 * 使用当前设置的字体、颜色和透明模式。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param x    字符串起始x坐标
 * @param y    字符串起始y坐标（取决于字体位置模式）
 * @param str  要绘制的ASCII字符串
 * @return 字符串的总像素宽度
 *
 * @note 仅支持ASCII字符(0-127)，如需绘制中文等请使用u8g2_DrawUTF8
 */
u8g2_uint_t u8g2_DrawStr(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_ascii_next;
  return u8g2_draw_string(u8g2, x, y, str);
}

/**
 * @brief 绘制2倍放大的ASCII字符串
 *
 * 与u8g2_DrawStr功能相同，但字符放大2倍显示。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param x    字符串起始x坐标
 * @param y    字符串起始y坐标
 * @param str  要绘制的ASCII字符串
 * @return 字符串的总像素宽度（2倍值）
 */
u8g2_uint_t u8g2_DrawStrX2(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_ascii_next;
  return u8g2_draw_string_2x(u8g2, x, y, str);
}

/*
UTF-8编码说明：
source: https://en.wikipedia.org/wiki/UTF-8
Bits  from          to            bytes  Byte 1    Byte 2    Byte 3    Byte 4    Byte 5    Byte 6
  7   U+0000        U+007F        1      0xxxxxxx
11    U+0080        U+07FF        2      110xxxxx  10xxxxxx
16    U+0800        U+FFFF        3      1110xxxx  10xxxxxx  10xxxxxx
21    U+10000       U+1FFFFF      4      11110xxx  10xxxxxx  10xxxxxx  10xxxxxx
26    U+200000      U+3FFFFFF     5      111110xx  10xxxxxx  10xxxxxx  10xxxxxx  10xxxxxx
31    U+4000000     U+7FFFFFFF    6      1111110x  10xxxxxx  10xxxxxx  10xxxxxx  10xxxxxx  10xxxxxx
*/

/**
 * @brief 绘制UTF-8编码的字符串
 *
 * 在指定位置绘制UTF-8编码的字符串。支持ASCII和Unicode字符。
 * 这是绘制多语言文本（如中文）的推荐函数。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param x    字符串起始x坐标
 * @param y    字符串起始y坐标
 * @param str  要绘制的UTF-8字符串
 * @return 字符串的总像素宽度
 *
 * @note 字体必须包含对应的Unicode字形才能正确显示
 * @note 可使用u8g2_IsAllValidUTF8预先检查字体是否支持所有字符
 */
u8g2_uint_t u8g2_DrawUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_utf8_next;
  return u8g2_draw_string(u8g2, x, y, str);
}

/**
 * @brief 绘制2倍放大的UTF-8字符串
 *
 * 与u8g2_DrawUTF8功能相同，但字符放大2倍显示。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param x    字符串起始x坐标
 * @param y    字符串起始y坐标
 * @param str  要绘制的UTF-8字符串
 * @return 字符串的总像素宽度（2倍值）
 */
u8g2_uint_t u8g2_DrawUTF8X2(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_utf8_next;
  return u8g2_draw_string_2x(u8g2, x, y, str);
}


/**
 * @brief 绘制带字距调整的UTF-8字符串
 *
 * 在绘制字符串时应用字距调整(Kerning)规则，改善字符间距的视觉效果。
 * 字距调整会根据相邻字符的组合自动调整间距。
 *
 * @param u8g2    u8g2设备结构体指针
 * @param x       字符串起始x坐标
 * @param y       字符串起始y坐标
 * @param to_left 绘制方向：0=从左到右，1=从右到左
 * @param kerning 字距调整数据结构指针
 * @param str     要绘制的UTF-8字符串
 * @return 字符串的总像素宽度
 *
 * @note 字距调整可使文本显示更美观，特别是对于西文字体
 */
u8g2_uint_t u8g2_DrawExtendedUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint8_t to_left, u8g2_kerning_t *kerning, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_utf8_next;
  uint16_t e_prev = 0x0ffff;
  uint16_t e;
  u8g2_uint_t delta, sum, k;
  u8x8_utf8_init(u8g2_GetU8x8(u8g2));
  sum = 0;
  for(;;)
  {
    e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      delta = u8g2_GetGlyphWidth(u8g2, e);
	    
      if ( to_left )
      {
        k = u8g2_GetKerning(u8g2, kerning, e, e_prev);
	delta -= k;
	x -= delta;
      }
      else
      {
        k = u8g2_GetKerning(u8g2, kerning, e_prev, e);
	delta -= k;
      }
      e_prev = e;

      u8g2_DrawGlyph(u8g2, x, y, e);
      if ( to_left )
      {
      }
      else
      {
	x += delta;
	x -= k;
      }
      
      sum += delta;    
    }
  }
  return sum;
}

/**
 * @brief 绘制带字距调整表的UTF-8字符串
 *
 * 与u8g2_DrawExtendedUTF8类似，但使用简化的字距调整表格式。
 *
 * @param u8g2          u8g2设备结构体指针
 * @param x             字符串起始x坐标
 * @param y             字符串起始y坐标
 * @param to_left       绘制方向：0=从左到右，1=从右到左
 * @param kerning_table 字距调整表指针（uint16_t数组格式）
 * @param str           要绘制的UTF-8字符串
 * @return 字符串的总像素宽度
 */
u8g2_uint_t u8g2_DrawExtUTF8(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, uint8_t to_left, const uint16_t *kerning_table, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_utf8_next;
  uint16_t e_prev = 0x0ffff;
  uint16_t e;
  u8g2_uint_t delta, sum, k;
  u8x8_utf8_init(u8g2_GetU8x8(u8g2));
  sum = 0;
  for(;;)
  {
    e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      delta = u8g2_GetGlyphWidth(u8g2, e);
	    
      if ( to_left )
      {
        k = u8g2_GetKerningByTable(u8g2, kerning_table, e, e_prev);
	delta -= k;
	x -= delta;
      }
      else
      {
        k = u8g2_GetKerningByTable(u8g2, kerning_table, e_prev, e);
	delta -= k;
      }
      e_prev = e;

      if ( to_left )
      {
      }
      else
      {
	x += delta;
      }
      u8g2_DrawGlyph(u8g2, x, y, e);
      if ( to_left )
      {
      }
      else
      {
	//x += delta;
	//x -= k;
      }
      
      sum += delta;    
    }
  }
  return sum;
}



/*===============================================*/
/* 字体高度参考点计算函数 */
/* 这些函数用于计算字体的上升(ascent)和下降(descent)高度， */
/* 影响垂直对齐和字符串边界框的计算 */

/**
 * @brief 更新字体的参考高度
 *
 * 根据当前的字体高度模式，计算并更新字体的上升和下降高度。
 * 这些值用于垂直对齐和字符串边界框计算。
 *
 * 三种高度模式：
 * - TEXT模式：仅使用大写A的上升和小写g的下降
 * - XTEXT模式：扩展使用括号的上升/下降
 * - ALL模式：使用最大字符高度
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_UpdateRefHeight(u8g2_t *u8g2)
{
  if ( u8g2->font == NULL )
    return;
  u8g2->font_ref_ascent = u8g2->font_info.ascent_A;
  u8g2->font_ref_descent = u8g2->font_info.descent_g;
  if ( u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_TEXT )
  {
  }
  else if ( u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_XTEXT )
  {
    if ( u8g2->font_ref_ascent < u8g2->font_info.ascent_para )
      u8g2->font_ref_ascent = u8g2->font_info.ascent_para;
    if ( u8g2->font_ref_descent > u8g2->font_info.descent_para )
      u8g2->font_ref_descent = u8g2->font_info.descent_para;
  }
  else
  {
    if ( u8g2->font_ref_ascent < u8g2->font_info.max_char_height+u8g2->font_info.y_offset )
      u8g2->font_ref_ascent = u8g2->font_info.max_char_height+u8g2->font_info.y_offset;
    if ( u8g2->font_ref_descent > u8g2->font_info.y_offset )
      u8g2->font_ref_descent = u8g2->font_info.y_offset;
  }  
}

/**
 * @brief 设置字体高度模式为TEXT模式
 *
 * TEXT模式仅使用大写字母A的上升高度和小写字母g的下降深度作为参考。
 * 这是默认的字体高度计算模式，适用于大多数西文文本。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontRefHeightText(u8g2_t *u8g2)
{
  u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_TEXT;
  u8g2_UpdateRefHeight(u8g2);
}

/**
 * @brief 设置字体高度模式为扩展TEXT模式
 *
 * XTEXT模式使用大写A、小写g以及括号的上升/下降高度作为参考。
 * 比TEXT模式考虑更多字符，适用于包含括号等字符的文本。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontRefHeightExtendedText(u8g2_t *u8g2)
{
  u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_XTEXT;
  u8g2_UpdateRefHeight(u8g2);
}

/**
 * @brief 设置字体高度模式为ALL模式
 *
 * ALL模式使用字体中最大字符的完整高度作为参考。
 * 确保所有字符都能完整显示，但可能导致行间距较大。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontRefHeightAll(u8g2_t *u8g2)
{
  u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_ALL;
  u8g2_UpdateRefHeight(u8g2);
}

/*===============================================*/
/* 字体位置回调函数 */
/* 这些函数计算字体绘制时的y坐标偏移量，实现不同的垂直对齐方式 */

/**
 * @brief 基线位置计算回调
 *
 * 基线模式下不需要偏移，返回0。
 * 基线是西文字体的标准对齐参考线。
 *
 * @param u8g2 u8g2设备结构体指针（未使用）
 * @return 始终返回0
 */
u8g2_uint_t u8g2_font_calc_vref_font(U8X8_UNUSED u8g2_t *u8g2)
{
  return 0;
}

/**
 * @brief 设置字体位置为基线模式
 *
 * 基线模式：y坐标表示字符基线的位置。
 * 字符的上升部分在基线上方，下降部分在基线下方。
 * 这是西文字体的标准对齐方式。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontPosBaseline(u8g2_t *u8g2)
{
  u8g2->font_calc_vref = u8g2_font_calc_vref_font;
}


/**
 * @brief 底部位置计算回调
 *
 * 计算字体下降部的偏移量，使y坐标对齐到字符底部。
 *
 * @param u8g2 u8g2设备结构体指针
 * @return 字体下降部的偏移量（通常为负值）
 */
u8g2_uint_t u8g2_font_calc_vref_bottom(u8g2_t *u8g2)
{
  return (u8g2_uint_t)(u8g2->font_ref_descent);
}

/**
 * @brief 设置字体位置为底部模式
 *
 * 底部模式：y坐标表示字符最底部（下降部底端）的位置。
 * 所有字符都在y坐标上方绘制。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontPosBottom(u8g2_t *u8g2)
{
  u8g2->font_calc_vref = u8g2_font_calc_vref_bottom;
}

/**
 * @brief 顶部位置计算回调
 *
 * 计算字体上升部的偏移量，使y坐标对齐到字符顶部。
 *
 * @param u8g2 u8g2设备结构体指针
 * @return 字体上升部的偏移量+1
 */
u8g2_uint_t u8g2_font_calc_vref_top(u8g2_t *u8g2)
{
  u8g2_uint_t tmp;
  /* reference pos is one pixel above the upper edge of the reference glyph */
  tmp = (u8g2_uint_t)(u8g2->font_ref_ascent);
  tmp++;
  return tmp;
}

/**
 * @brief 设置字体位置为顶部模式
 *
 * 顶部模式：y坐标表示字符最顶部（上升部顶端）的位置。
 * 所有字符都在y坐标下方绘制。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontPosTop(u8g2_t *u8g2)
{
  u8g2->font_calc_vref = u8g2_font_calc_vref_top;
}

/**
 * @brief 居中位置计算回调
 *
 * 计算字体垂直居中的偏移量，使y坐标对齐到字符垂直中心。
 * 居中位置 = (上升高度 + 下降深度) / 2 + 下降深度
 *
 * @param u8g2 u8g2设备结构体指针
 * @return 字体垂直居中的偏移量
 */
u8g2_uint_t u8g2_font_calc_vref_center(u8g2_t *u8g2)
{
  int8_t tmp;
  tmp = u8g2->font_ref_ascent;
  tmp -= u8g2->font_ref_descent;
  tmp /= 2;
  tmp += u8g2->font_ref_descent;  
  return tmp;
}

/**
 * @brief 设置字体位置为居中模式
 *
 * 居中模式：y坐标表示字符垂直中心的位置。
 * 适用于需要垂直居中对齐文本的场景。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetFontPosCenter(u8g2_t *u8g2)
{
  u8g2->font_calc_vref = u8g2_font_calc_vref_center;
}

/*===============================================*/
/* 字体设置和查询函数 */

/**
 * @brief 设置当前字体
 *
 * 设置u8g2对象使用的字体。会读取字体头部信息并更新参考高度。
 * 如果设置的字体与当前字体相同，则不做任何操作。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param font 字体数据指针（通常由字体生成工具生成的数组）
 *
 * @note 字体数据存储在Flash中，通过const数组访问
 * @note 切换字体后，之前的字体位置设置仍然有效
 */
void u8g2_SetFont(u8g2_t *u8g2, const uint8_t  *font)
{
  if ( u8g2->font != font )
  {
//#ifdef  __unix__
//	u8g2->last_font_data = NULL;
//	u8g2->last_unicode = 0x0ffff;
//#endif 
    u8g2->font = font;
    u8g2_read_font_info(&(u8g2->font_info), font);
    u8g2_UpdateRefHeight(u8g2);
    /* u8g2_SetFontPosBaseline(u8g2); */ /* removed with issue 195 */
  }
}

/*===============================================*/
/* 字符验证函数 */

/**
 * @brief 检查字符串中的所有字符是否都有对应的字形（内部函数）
 *
 * 遍历字符串中的每个字符，检查当前字体是否包含对应的字形数据。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param str  要检查的字符串
 * @return 1 表示所有字符都有字形，0 表示存在缺失的字形
 */
static uint8_t u8g2_is_all_valid(u8g2_t *u8g2, const char *str) U8G2_NOINLINE;
static uint8_t u8g2_is_all_valid(u8g2_t *u8g2, const char *str)
{
  uint16_t e;
  u8x8_utf8_init(u8g2_GetU8x8(u8g2));
  for(;;)
  {
    e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      if ( u8g2_font_get_glyph_data(u8g2, e) == NULL )
	return 0;
    }
  }
  return 1;
}

/**
 * @brief 检查UTF-8字符串中的所有字符是否都有对应的字形
 *
 * 在绘制UTF-8字符串前，可用此函数验证字体是否支持所有字符。
 * 常用于多语言环境，确保字体包含所需的Unicode字形。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param str  要检查的UTF-8字符串
 * @return 1 表示所有字符都有字形，0 表示存在缺失的字形
 */
uint8_t u8g2_IsAllValidUTF8(u8g2_t *u8g2, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_utf8_next;
  return u8g2_is_all_valid(u8g2, str);
}


/*===============================================*/
/* 字符串宽度计算函数 */

/**
 * @brief 计算字符串的像素宽度（内部函数）
 *
 * 遍历字符串中的每个字符，累加其水平推进距离，计算字符串的总像素宽度。
 * 宽度计算考虑了字符间距和字形的实际像素宽度。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param str  要计算宽度的字符串
 * @return 字符串的总像素宽度
 *
 * @note 计算结果包含第一个字符的x偏移量
 * @note 可通过U8G2_BALANCED_STR_WIDTH_CALCULATION宏启用平衡宽度计算
 */
static u8g2_uint_t u8g2_string_width(u8g2_t *u8g2, const char *str) U8G2_NOINLINE;
static u8g2_uint_t u8g2_string_width(u8g2_t *u8g2, const char *str)
{
  uint16_t e;
  u8g2_uint_t  w, dx;
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
  int8_t initial_x_offset = -64;
#endif 
  
  u8g2->font_decode.glyph_width = 0;
  u8x8_utf8_init(u8g2_GetU8x8(u8g2));
  
  /* reset the total width to zero, this will be expanded during calculation */
  w = 0;
  dx = 0;

  // printf("str=<%s>\n", str);
	
  for(;;)
  {
    e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( e == 0x0ffff )
      break;
    str++;
    if ( e != 0x0fffe )
    {
      dx = u8g2_GetGlyphWidth(u8g2, e);		/* delta x value of the glyph */
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
      if ( initial_x_offset == -64 )
        initial_x_offset = u8g2->glyph_x_offset;
#endif 
      //printf("'%c' x=%d dx=%d w=%d io=%d ", e, u8g2->glyph_x_offset, dx, u8g2->font_decode.glyph_width, initial_x_offset);
      w += dx;
    }
  }
  //printf("\n");
  
  /* adjust the last glyph, check for issue #16: do not adjust if width is 0 */
  if ( u8g2->font_decode.glyph_width != 0 )
  {
    //printf("string width adjust dx=%d glyph_width=%d x-offset=%d\n", dx, u8g2->font_decode.glyph_width, u8g2->glyph_x_offset);
    w -= dx;
    w += u8g2->font_decode.glyph_width;  /* the real pixel width of the glyph, sideeffect of GetGlyphWidth */
    /* issue #46: we have to add the x offset also */
    w += u8g2->glyph_x_offset;	/* this value is set as a side effect of u8g2_GetGlyphWidth() */
#ifdef U8G2_BALANCED_STR_WIDTH_CALCULATION
    /* https://github.com/olikraus/u8g2/issues/1561 */
    if ( initial_x_offset > 0 )
      w+=initial_x_offset;
#endif 
  }
  // printf("w=%d \n", w);
  
  return w;  
}

/**
 * @brief 获取字形的水平属性（内部函数）
 *
 * 获取指定字符的宽度、x偏移和水平推进距离等水平属性。
 *
 * @param u8g2              u8g2设备结构体指针
 * @param requested_encoding 字符编码
 * @param w [输出]          字形的实际像素宽度
 * @param ox [输出]         字形的x方向偏移量
 * @param dx [输出]         字形的水平推进距离
 */
static void u8g2_GetGlyphHorizontalProperties(u8g2_t *u8g2, uint16_t requested_encoding, uint8_t *w, int8_t *ox, int8_t *dx)
{
  const uint8_t *glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
  if ( glyph_data == NULL )
    return; 
  
  u8g2_font_setup_decode(u8g2, glyph_data);
  *w = u8g2->font_decode.glyph_width;
  *ox =  u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
  u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);
  *dx = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}

/**
 * @brief 获取字符串第一个字符的x偏移量
 *
 * 获取字符串第一个字符的x方向偏移量，用于精确计算字符串的起始位置。
 * 该函数与u8g库兼容。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param s    字符串指针
 * @return 第一个字符的x偏移量
 */
int8_t u8g2_GetStrX(u8g2_t *u8g2, const char *s)
{
  uint8_t w;
  int8_t dx;
  int8_t ox = 0;
  u8g2_GetGlyphHorizontalProperties(u8g2, *s, &w, &ox, &dx);
  return ox;
}


/*
Warning: This function needs to be fixed. I think it was taken over from u8glib, but not fixed as of now 
The main difference for this procedure compared to the normal get width, should be, that the initial
offset is removed

Idea: for the user interface it probably would be better to add the xoffset of the first char to the end, so that the overall word looks better.
Maybe then the procedure should be called differently, maybe balanced width instead of exact width

u8g2_calculate_exact_string_width is now OBSOLETE, instead the above str width calculation has been updated:
https://github.com/olikraus/u8g2/issues/1561
*/
#ifdef OBSOLETE
static u8g2_uint_t u8g2_calculate_exact_string_width(u8g2_t *u8g2, const char *str)
{
  const char *s = str;
  uint16_t enc;
  u8g2_uint_t  w;
  uint8_t cnt;
  uint8_t gw; 
  int8_t ox, dx;
  
  /* reset the total minimal width to zero, this will be expanded during calculation */
  w = 0;
    
  
  /* check for empty string, width is already 0 */
  cnt = 0;
  
  for(;;)
  {
    enc = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*s);
    if ( enc == 0x0ffff )
      break;    
    s++;
    if ( enc != 0x0fffe )
    {
      if ( cnt == 0 )
      {
        /* get glyph properties of the first char */
        u8g2_GetGlyphHorizontalProperties(u8g2, enc, &gw, &ox, &dx);  
      }        
      cnt++;
      if ( cnt > 2 )
        break;
    }
  }
  
  if ( cnt == 0 )
    return 0;
   
  if ( cnt == 1 )
     return gw;

  /* strlen(s) == 1:       width = width(s[0]) */
  /* strlen(s) == 2:       width = - offx(s[0]) + deltax(s[0]) + offx(s[1]) + width(s[1]) */
  /* strlen(s) == 3:       width = - offx(s[0]) + deltax(s[0]) + deltax(s[1]) + offx(s[2]) + width(s[2]) */
  
  /* assume that the string has size 2 or more, than start with negative offset-x */
  /* for string with size 1, this will be nullified after the loop */
  w = -ox;  
  for(;;)
  {
    enc = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
    if ( enc== 0x0ffff )
      break;
    str++;
    if ( enc != 0x0fffe )
    {
      u8g2_GetGlyphHorizontalProperties(u8g2, enc, &gw, &ox, &dx);        
      /* if there are still more characters, add the delta to the next glyph */
      w += dx;    
    }
  }
  
  /* finally calculate the width of the last char */
  /* here is another exception, if the last char is a blank, use the dx value instead */
  if ( gw != 0 )
  {
    w -= dx;    /* remove the last dx */
    /* if g was not updated in the for loop (strlen() == 1), then the initial offset x gets removed */
    w += gw;
    w += ox;
  }
  else
  {
    //w += dx;
  }
  
  
  return w;
	
}
#endif





/**
 * @brief 计算ASCII字符串的像素宽度
 *
 * 计算以ASCII编码的字符串在当前字体下占用的像素宽度。
 * 常用于文本居中对齐或布局计算。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param s    要计算宽度的ASCII字符串
 * @return 字符串的总像素宽度
 *
 * @note 仅适用于ASCII字符串，UTF-8字符串请使用u8g2_GetUTF8Width
 */
u8g2_uint_t u8g2_GetStrWidth(u8g2_t *u8g2, const char *s)
{
  u8g2->u8x8.next_cb = u8x8_ascii_next;
  return u8g2_string_width(u8g2, s);
}

/* OBSOLETE
u8g2_uint_t u8g2_GetExactStrWidth(u8g2_t *u8g2, const char *s)
{
  u8g2->u8x8.next_cb = u8x8_ascii_next;
  return u8g2_calculate_exact_string_width(u8g2, s);
}
*/

/**
 * @brief 计算UTF-8字符串的像素宽度
 *
 * 计算以UTF-8编码的字符串在当前字体下占用的像素宽度。
 * 支持ASCII和Unicode字符，适用于多语言文本。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param str  要计算宽度的UTF-8字符串
 * @return 字符串的总像素宽度
 */
u8g2_uint_t u8g2_GetUTF8Width(u8g2_t *u8g2, const char *str)
{
  u8g2->u8x8.next_cb = u8x8_utf8_next;
  return u8g2_string_width(u8g2, str);
}



/**
 * @brief 设置字体绘制方向
 *
 * 设置字符和字符串的绘制方向，支持四个方向的旋转。
 * 仅在编译时启用U8G2_WITH_FONT_ROTATION宏时有效。
 *
 * @param u8g2 u8g2设备结构体指针
 * @param dir  绘制方向：
 *             - 0: 正常方向（从左到右）
 *             - 1: 顺时针旋转90度
 *             - 2: 旋转180度（上下颠倒）
 *             - 3: 逆时针旋转90度
 */
void u8g2_SetFontDirection(u8g2_t *u8g2, uint8_t dir)
{
#ifdef U8G2_WITH_FONT_ROTATION  
  u8g2->font_decode.dir = dir;
#endif
}


