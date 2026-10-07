/**
 * @file u8x8_8x8.c
 * @brief u8x8字体渲染与显示核心文件
 *
 * 本文件实现了u8x8库的字体相关功能，包括：
 * - 字体设置与字形数据获取
 * - 字形绘制（1x1、2x2、1x2缩放模式）
 * - UTF-8编码解码状态机
 * - 字符串绘制（ASCII和UTF-8模式）
 * - 位交错算法用于字形放大
 *
 * 该文件直接与底层显示过程接口，是u8x8显示库的核心组件之一。
 * 适用于STM32智能手表项目的OLED/LCD屏幕驱动。
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

/**
 * @brief ESP8266平台的程序存储器读取函数
 *
 * 由于ESP8266不支持PROGMEM，需要通过此函数从Flash读取数据。
 * 该函数处理非对齐地址访问，确保正确读取字节数据。
 *
 * @param addr 要读取的Flash地址
 * @return 读取到的字节值
 */
#if defined(ESP8266)
uint8_t u8x8_pgm_read_esp(const uint8_t * addr)
{
    uint32_t bytes;
    /* 将地址对齐到4字节边界并读取32位数据 */
    bytes = *(uint32_t*)((uint32_t)addr & ~3);
    /* 根据地址的低2位选择对应的字节 */
    return ((uint8_t*)&bytes)[(uint32_t)addr & 3];
}
#endif


/**
 * @brief 设置u8x8显示结构体使用的字体
 *
 * 将指定的8x8字体数据指针赋值给u8x8结构体的font成员。
 * 字体数据通常存储在程序存储器(Flash)中，包含字形的位图数据。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param font_8x8 指向8x8字体数据的指针（通常在Flash中）
 */
void u8x8_SetFont(u8x8_t *u8x8, const uint8_t *font_8x8)
{
  u8x8->font = font_8x8;  /* 保存字体数据指针 */
}

/**
 * @brief 获取指定编码字符的字形位图数据
 *
 * 从字体数据中读取指定编码字符的8字节位图数据。
 * 字体数据格式（2019版）：
 *   - 偏移0: 第一个字符编码 (first)
 *   - 偏移1: 最后一个字符编码 (last)
 *   - 偏移2: 水平tile数量 (th)
 *   - 偏移3: 垂直tile数量 (tv)
 *   - 偏移4+: 字形位图数据
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param encoding 要获取的字符编码（0-255）
 * @param buf 指向8字节缓冲区的指针，用于存储位图数据
 * @param tile_offset tile偏移量，用于多tile字形
 */
static void u8x8_get_glyph_data(u8x8_t *u8x8, uint8_t encoding, uint8_t *buf, uint8_t tile_offset) U8X8_NOINLINE;
static void u8x8_get_glyph_data(u8x8_t *u8x8, uint8_t encoding, uint8_t *buf, uint8_t tile_offset)
{
  uint8_t first, last, tiles, i;
  uint16_t offset;
  /* 从字体头部读取字符范围和tile信息 */
  first = u8x8_pgm_read(u8x8->font+0);    /* 字体中第一个字符的编码 */
  last = u8x8_pgm_read(u8x8->font+1);     /* 字体中最后一个字符的编码 */
  tiles = u8x8_pgm_read(u8x8->font+2);    /* 水平方向tile数量 */
  tiles *= u8x8_pgm_read(u8x8->font+3);   /* 总tile数 = 水平 × 垂直 */

  /* 检查编码是否在字体支持的范围内 */
  if ( first <= encoding && encoding <= last )
  {
    /* 计算字形数据在字体数组中的偏移量 */
    offset = encoding;
    offset -= first;           /* 相对于第一个字符的偏移 */
    offset *= tiles;           /* 乘以每个字符的tile数 */
    offset += tile_offset;     /* 加上tile偏移 */
    offset *= 8;               /* 每个tile 8字节 */
    offset +=4;                /* 跳过字体头部4字节（2019格式） */
    /* 读取8字节位图数据 */
    for( i = 0; i < 8; i++ )
    {
      buf[i] = u8x8_pgm_read(u8x8->font+offset);
      offset++;
    }
  }
  else
  {
    /* 编码超出范围，返回空白字形（全0） */
    for( i = 0; i < 8; i++ )
    {
      buf[i] = 0;
    }
  }

  /* 如果启用了反显模式，对位图数据取反 */
  if ( u8x8->is_font_inverse_mode )
  {
    for( i = 0; i < 8; i++ )
    {
      buf[i] ^= 255;  /* 按位异或取反 */
    }
  }

}

/**
 * @brief 在指定位置绘制单个字符（1:1比例）
 *
 * 使用当前字体在指定的tile坐标位置绘制一个字符。
 * 支持多tile字形（字符宽度>8像素或高度>8像素的情况）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 水平tile坐标（单位：tile，1 tile = 8像素）
 * @param y 垂直tile坐标（单位：tile，1 tile = 8像素）
 * @param encoding 要绘制的字符编码
 */
void u8x8_DrawGlyph(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t encoding)
{
  uint8_t th = u8x8_pgm_read(u8x8->font+2);    /* 字形水平tile数 */
  uint8_t tv = u8x8_pgm_read(u8x8->font+3);    /* 字形垂直tile数 */
  uint8_t xx, tile;
  uint8_t buf[8];
  th += x;   /* 计算右边界 */
  tv += y;   /* 计算下边界 */
  tile = 0;  /* tile索引从0开始 */
  /* 双重循环遍历字形的所有tile */
  do
  {
    xx = x;
    do
    {
      u8x8_get_glyph_data(u8x8, encoding, buf, tile);  /* 获取当前tile的位图 */
      u8x8_DrawTile(u8x8, xx, y, 1, buf);              /* 绘制单个tile */
      tile++;
      xx++;
    } while( xx < th );  /* 水平方向循环 */
    y++;
  } while( y < tv );    /* 垂直方向循环 */
}


/**
 * @brief 将8位字节扩展为16位（位交错算法）
 *
 * 使用二进制魔数位交错技术，将8位数据的每个位扩展为2位。
 * 例如：输入 0b10101010 -> 输出 0b1100110011001100
 * 该算法用于实现字形的2倍水平放大。
 *
 * 算法来源: http://graphics.stanford.edu/~seander/bithacks.html
 * Section: Interleave bits by Binary Magic Numbers
 *
 * @param x 要扩展的8位字节
 * @return 扩展后的16位值，每个原始位变为2位
 */
uint16_t u8x8_upscale_byte(uint8_t x)
{
	uint16_t y = x;
	/* 第一步：将位分散到偶数位置 */
	y |= (y << 4);    /* 将高4位复制到低4位的间隙 */
	y &= 0x0f0f;      /* 掩码保留有效位 */
	y |= (y << 2);    /* 进一步分散位 */
	y &= 0x3333;      /* 掩码保留有效位 */
	y |= (y << 1);    /* 最终分散到位0,2,4,6... */
	y &= 0x5555;      /* 掩码保留偶数位 */

	/* 第二步：将偶数位复制到奇数位置，实现每个位扩展为2位 */
	y |= (y << 1);    /* z = x | (y << 1) */
	return y;
}

/**
 * @brief 将4字节缓冲区扩展为8字节（每字节复制一次）
 *
 * 将源缓冲区的每个字节复制两次到目标缓冲区，实现2倍垂直放大。
 * 用于配合u8x8_upscale_byte实现字形的2x2放大显示。
 *
 * @param src 指向4字节源缓冲区的指针
 * @param dest 指向8字节目标缓冲区的指针
 */
static void u8x8_upscale_buf(uint8_t *src, uint8_t *dest) U8X8_NOINLINE;
static void u8x8_upscale_buf(uint8_t *src, uint8_t *dest)
{
  uint8_t i = 4;
  do
  {
    *dest++ = *src;    /* 第一次复制 */
    *dest++ = *src++;  /* 第二次复制，并移动源指针 */
    i--;
  } while( i > 0 );
}

/**
 * @brief 绘制2x2放大的子字形（单个tile放大为4个tile）
 *
 * 将一个8x8的字形tile放大为16x16像素（2x2个tile）显示。
 * 使用位交错算法实现水平放大，字节复制实现垂直放大。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 水平tile坐标
 * @param y 垂直tile坐标
 * @param encoding 字符编码
 * @param tile tile索引
 */
static void u8x8_draw_2x2_subglyph(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t encoding, uint8_t tile)
{
  uint8_t i;
  uint16_t t;
  uint8_t buf[8];
  uint8_t buf1[8];  /* 存放上半部分放大后的数据 */
  uint8_t buf2[8];  /* 存放下半部分放大后的数据 */
  /* 获取原始字形数据 */
  u8x8_get_glyph_data(u8x8, encoding, buf, tile);
  /* 对每一行进行水平2倍放大 */
  for( i = 0; i < 8; i ++ )
  {
      t = u8x8_upscale_byte(buf[i]);  /* 水平放大：8位->16位 */
      buf1[i] = t >> 8;               /* 高8位（左半部分） */
      buf2[i] = t & 255;              /* 低8位（右半部分） */
  }
  /* 绘制左上角tile（下部的右半） */
  u8x8_upscale_buf(buf2, buf);
  u8x8_DrawTile(u8x8, x, y, 1, buf);

  /* 绘制右上角tile（上部的右半） */
  u8x8_upscale_buf(buf2+4, buf);
  u8x8_DrawTile(u8x8, x+1, y, 1, buf);

  /* 绘制左下角tile（下部的左半） */
  u8x8_upscale_buf(buf1, buf);
  u8x8_DrawTile(u8x8, x, y+1, 1, buf);

  /* 绘制右下角tile（上部的左半） */
  u8x8_upscale_buf(buf1+4, buf);
  u8x8_DrawTile(u8x8, x+1, y+1, 1, buf);
}


/**
 * @brief 在指定位置绘制2倍放大的字符
 *
 * 将字符放大2倍后绘制到屏幕上。适用于需要大字体显示的场景，
 * 如智能手表的时间显示、标题等。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 水平tile坐标
 * @param y 垂直tile坐标
 * @param encoding 要绘制的字符编码
 */
void u8x8_Draw2x2Glyph(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t encoding)
{
  uint8_t th = u8x8_pgm_read(u8x8->font+2);    /* 字形水平tile数 */
  uint8_t tv = u8x8_pgm_read(u8x8->font+3);    /* 字形垂直tile数 */
  uint8_t xx, tile;
  th *= 2;     /* 放大后的水平tile数 */
  th += x;     /* 计算右边界 */
  tv *= 2;     /* 放大后的垂直tile数 */
  tv += y;     /* 计算下边界 */
  tile = 0;    /* tile索引从0开始 */
  /* 双重循环遍历放大后的所有tile位置 */
  do
  {
    xx = x;
    do
    {
      u8x8_draw_2x2_subglyph(u8x8, xx, y, encoding, tile);  /* 绘制2x2子字形 */
      tile++;
      xx+=2;   /* 每次水平移动2个tile */
    } while( xx < th );
    y+=2;      /* 每次垂直移动2个tile */
  } while( y < tv );
}

/**
 * @brief 绘制1x2放大的子字形（单个tile垂直放大为2个tile）
 *
 * 将一个8x8的字形tile垂直放大2倍，水平保持不变。
 * 参考: https://github.com/olikraus/u8g2/issues/474
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 水平tile坐标
 * @param y 垂直tile坐标
 * @param encoding 字符编码
 * @param tile tile索引
 */
static void u8x8_draw_1x2_subglyph(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t encoding, uint8_t tile)
{
  uint8_t i;
  uint16_t t;
  uint8_t buf[8];
  uint8_t buf1[8];  /* 存放上半部分数据 */
  uint8_t buf2[8];  /* 存放下半部分数据 */
  /* 获取原始字形数据 */
  u8x8_get_glyph_data(u8x8, encoding, buf, tile);
  /* 对每一行进行水平2倍放大（用于后续垂直复制） */
  for( i = 0; i < 8; i ++ )
  {
      t = u8x8_upscale_byte(buf[i]);  /* 水平放大：8位->16位 */
      buf1[i] = t >> 8;               /* 高8位 */
      buf2[i] = t & 255;              /* 低8位 */
  }
  /* 绘制上半部分tile */
  u8x8_DrawTile(u8x8, x,   y, 1, buf2);
  /* 绘制下半部分tile */
  u8x8_DrawTile(u8x8, x, y+1, 1, buf1);
}

/**
 * @brief 在指定位置绘制1x2放大的字符（宽度不变，高度2倍）
 *
 * 将字符垂直放大2倍，水平保持原始宽度。
 * 适用于需要垂直拉伸显示的场景。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 水平tile坐标
 * @param y 垂直tile坐标
 * @param encoding 要绘制的字符编码
 */
void u8x8_Draw1x2Glyph(u8x8_t *u8x8, uint8_t x, uint8_t y, uint8_t encoding)
{
  uint8_t th = u8x8_pgm_read(u8x8->font+2);    /* 字形水平tile数 */
  uint8_t tv = u8x8_pgm_read(u8x8->font+3);    /* 字形垂直tile数 */
  uint8_t xx, tile;
  th += x;       /* 计算右边界（水平不放大） */
  tv *= 2;       /* 垂直放大2倍 */
  tv += y;       /* 计算下边界 */
  tile = 0;      /* tile索引从0开始 */
  /* 双重循环遍历所有tile位置 */
  do
  {
    xx = x;
    do
    {
      u8x8_draw_1x2_subglyph(u8x8, xx, y, encoding, tile);  /* 绘制1x2子字形 */
      tile++;
      xx++;    /* 水平每次移动1个tile */
    } while( xx < th );
    y+=2;      /* 垂直每次移动2个tile */
  } while( y < tv );
}

/**
 * @brief UTF-8编码格式说明
 *
 * 来源: https://en.wikipedia.org/wiki/UTF-8
 * UTF-8是一种变长字符编码，编码规则如下：
 * - 1字节: 0xxxxxxx (U+0000 ~ U+007F) - ASCII字符
 * - 2字节: 110xxxxx 10xxxxxx (U+0080 ~ U+07FF)
 * - 3字节: 1110xxxx 10xxxxxx 10xxxxxx (U+0800 ~ U+FFFF)
 * - 4字节: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (U+10000 ~ U+1FFFFF)
 * - 5字节: 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * - 6字节: 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

/**
 * @brief 重置UTF-8解码器状态机
 *
 * 在开始解码新的UTF-8字符串前调用此函数，将状态机重置到初始状态。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 */
void u8x8_utf8_init(u8x8_t *u8x8)
{
  u8x8->utf8_state = 0;  /* 重置UTF-8状态机，在u8x8_SetupDefaults()中也会重置 */
}

/**
 * @brief ASCII模式下的字符获取回调函数
 *
 * 逐个返回ASCII字符串中的字符。遇到字符串结束符'\0'或换行符'\n'时
 * 返回0x0ffff表示字符串结束。换行符用于支持字符串列表功能。
 *
 * @param u8x8 指向u8x8显示结构体的指针（未使用）
 * @param b 当前字节
 * @return 字符编码值，0x0ffff表示字符串结束
 */
uint16_t u8x8_ascii_next(U8X8_UNUSED u8x8_t *u8x8, uint8_t b)
{
  if ( b == 0 || b == '\n' ) /* '\n'结束字符串，以支持字符串列表功能 */
    return 0x0ffff;  /* 检测到字符串结束 */
  return b;
}

/**
 * @brief UTF-8解码状态机回调函数
 *
 * 将UTF-8编码字符串中的字节逐个传入状态机进行解码。
 * 返回值含义：
 *   - 0x0fffe: 还需要更多字节，继续解码
 *   - 0x0ffff: 字符串结束
 *   - 其他值: 解码后的Unicode编码点
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param b 当前字节
 * @return 解码结果（0x0fffe继续，0x0ffff结束，其他为字符编码）
 */
uint16_t u8x8_utf8_next(u8x8_t *u8x8, uint8_t b)
{
  if ( b == 0 || b == '\n' )	/* '\n'结束字符串，以支持字符串列表功能 */
    return 0x0ffff;	/* 检测到字符串结束，丢弃待处理的UTF8数据 */
  if ( u8x8->utf8_state == 0 )
  {
    /* 初始状态：分析首字节确定UTF-8序列长度 */
    if ( b >= 0xfc )	/* 6字节序列 */
    {
      u8x8->utf8_state = 5;
      b &= 1;          /* 提取有效位 */
    }
    else if ( b >= 0xf8 )  /* 5字节序列 */
    {
      u8x8->utf8_state = 4;
      b &= 3;
    }
    else if ( b >= 0xf0 )  /* 4字节序列 */
    {
      u8x8->utf8_state = 3;
      b &= 7;
    }
    else if ( b >= 0xe0 )  /* 3字节序列 */
    {
      u8x8->utf8_state = 2;
      b &= 15;
    }
    else if ( b >= 0xc0 )  /* 2字节序列 */
    {
      u8x8->utf8_state = 1;
      b &= 0x01f;
    }
    else
    {
      /* 单字节ASCII字符，直接返回 */
      return b;
    }
    u8x8->encoding = b;    /* 保存首字节的有效位 */
    return 0x0fffe;        /* 需要更多字节 */
  }
  else
  {
    /* 后续字节状态：处理10xxxxxx格式的延续字节 */
    u8x8->utf8_state--;
    /* 注意：未检查b < 0x080的非法UTF-8编码情况 */
    u8x8->encoding<<=6;    /* 左移6位为新字节腾出空间 */
    b &= 0x03f;            /* 提取6位有效数据 */
    u8x8->encoding |= b;   /* 合并到编码值中 */
    if ( u8x8->utf8_state != 0 )
      return 0x0fffe;      /* 还需要更多字节 */
  }
  return u8x8->encoding;   /* 返回完整的Unicode编码点 */
}



/**
 * @brief 内部字符串绘制函数（1:1比例）
 *
 * 使用当前字体和字符回调函数绘制字符串。
 * 支持ASCII和UTF-8编码模式（通过next_cb回调区分）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的字符串指针
 * @return 成功绘制的字符数量
 */
static uint8_t u8x8_draw_string(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s) U8X8_NOINLINE;
static uint8_t u8x8_draw_string(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  uint16_t e;
  uint8_t cnt = 0;
  uint8_t th = u8x8_pgm_read(u8x8->font+2);    /* 获取字形水平宽度 */

  u8x8_utf8_init(u8x8);  /* 初始化UTF-8解码器 */
  for(;;)
  {
    e = u8x8->next_cb(u8x8, (uint8_t)*s);  /* 调用回调获取下一个字符编码 */
    if ( e == 0x0ffff )     /* 字符串结束 */
      break;
    s++;
    if ( e != 0x0fffe )     /* 跳过UTF-8中间字节 */
    {
      u8x8_DrawGlyph(u8x8, x, y, e);  /* 绘制字符 */
      x+=th;    /* 水平位置前进一个字形宽度 */
      cnt++;    /* 计数器递增 */
    }
  }
  return cnt;
}


/**
 * @brief 在指定位置绘制ASCII字符串（1:1比例）
 *
 * 使用当前字体绘制ASCII编码的字符串。每个字符占8像素宽。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的ASCII字符串
 * @return 成功绘制的字符数量
 */
uint8_t u8x8_DrawString(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  u8x8->next_cb = u8x8_ascii_next;  /* 设置ASCII字符获取回调 */
  return u8x8_draw_string(u8x8, x, y, s);
}

/**
 * @brief 在指定位置绘制UTF-8字符串（1:1比例）
 *
 * 使用当前字体绘制UTF-8编码的字符串。支持多字节Unicode字符。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的UTF-8字符串
 * @return 成功绘制的字符数量
 */
uint8_t u8x8_DrawUTF8(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  u8x8->next_cb = u8x8_utf8_next;  /* 设置UTF-8字符获取回调 */
  return u8x8_draw_string(u8x8, x, y, s);
}



/**
 * @brief 内部2倍放大字符串绘制函数
 *
 * 使用当前字体绘制2倍放大的字符串。
 * 每个字符的宽度和高度都放大2倍。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的字符串指针
 * @return 成功绘制的字符数量
 */
static uint8_t u8x8_draw_2x2_string(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s) U8X8_NOINLINE;
static uint8_t u8x8_draw_2x2_string(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  uint16_t e;
  uint8_t cnt = 0;
  uint8_t th = u8x8_pgm_read(u8x8->font+2);  /* 获取字形原始水平宽度 */

  th <<= 1;  /* 水平宽度乘以2（2倍放大） */

  u8x8_utf8_init(u8x8);  /* 初始化UTF-8解码器 */
  for(;;)
  {
    e = u8x8->next_cb(u8x8, (uint8_t)*s);  /* 获取下一个字符编码 */
    if ( e == 0x0ffff )     /* 字符串结束 */
      break;
    s++;
    if ( e != 0x0fffe )     /* 跳过UTF-8中间字节 */
    {
      u8x8_Draw2x2Glyph(u8x8, x, y, e);  /* 绘制2倍放大的字符 */
      x+=th;    /* 水平位置前进（2倍宽度） */
      cnt++;    /* 计数器递增 */
    }
  }
  return cnt;
}


/**
 * @brief 在指定位置绘制2倍放大的ASCII字符串
 *
 * 使用当前字体绘制2倍放大的ASCII字符串。
 * 每个字符显示为16x16像素（2x2个tile）。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的ASCII字符串
 * @return 成功绘制的字符数量
 */
uint8_t u8x8_Draw2x2String(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  u8x8->next_cb = u8x8_ascii_next;  /* 设置ASCII字符获取回调 */
  return u8x8_draw_2x2_string(u8x8, x, y, s);
}

/**
 * @brief 在指定位置绘制2倍放大的UTF-8字符串
 *
 * 使用当前字体绘制2倍放大的UTF-8字符串。
 * 支持多字节Unicode字符的放大显示。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的UTF-8字符串
 * @return 成功绘制的字符数量
 */
uint8_t u8x8_Draw2x2UTF8(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  u8x8->next_cb = u8x8_utf8_next;  /* 设置UTF-8字符获取回调 */
  return u8x8_draw_2x2_string(u8x8, x, y, s);
}



/**
 * @brief 内部1x2放大字符串绘制函数
 *
 * 使用当前字体绘制垂直2倍放大的字符串。
 * 水平宽度保持不变，垂直高度放大2倍。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的字符串指针
 * @return 成功绘制的字符数量
 */
static uint8_t u8x8_draw_1x2_string(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s) U8X8_NOINLINE;
static uint8_t u8x8_draw_1x2_string(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  uint16_t e;
  uint8_t cnt = 0;
  uint8_t th = u8x8_pgm_read(u8x8->font+2);  /* 获取字形原始水平宽度 */
  u8x8_utf8_init(u8x8);  /* 初始化UTF-8解码器 */
  for(;;)
  {
    e = u8x8->next_cb(u8x8, (uint8_t)*s);  /* 获取下一个字符编码 */
    if ( e == 0x0ffff )     /* 字符串结束 */
      break;
    s++;
    if ( e != 0x0fffe )     /* 跳过UTF-8中间字节 */
    {
      u8x8_Draw1x2Glyph(u8x8, x, y, e);  /* 绘制1x2放大的字符 */
      x+=th;    /* 水平位置前进（原始宽度） */
      cnt++;    /* 计数器递增 */
    }
  }
  return cnt;
}


/**
 * @brief 在指定位置绘制1x2放大的ASCII字符串
 *
 * 使用当前字体绘制垂直2倍放大的ASCII字符串。
 * 水平宽度保持8像素，垂直高度放大为16像素。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的ASCII字符串
 * @return 成功绘制的字符数量
 */
uint8_t u8x8_Draw1x2String(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  u8x8->next_cb = u8x8_ascii_next;  /* 设置ASCII字符获取回调 */
  return u8x8_draw_1x2_string(u8x8, x, y, s);
}

/**
 * @brief 在指定位置绘制1x2放大的UTF-8字符串
 *
 * 使用当前字体绘制垂直2倍放大的UTF-8字符串。
 * 支持多字节Unicode字符的垂直拉伸显示。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param x 起始水平tile坐标
 * @param y 起始垂直tile坐标
 * @param s 要绘制的UTF-8字符串
 * @return 成功绘制的字符数量
 */
uint8_t u8x8_Draw1x2UTF8(u8x8_t *u8x8, uint8_t x, uint8_t y, const char *s)
{
  u8x8->next_cb = u8x8_utf8_next;  /* 设置UTF-8字符获取回调 */
  return u8x8_draw_1x2_string(u8x8, x, y, s);
}



/**
 * @brief 获取UTF-8字符串中的字符数量
 *
 * 计算UTF-8编码字符串中实际的字符（Unicode编码点）数量。
 * 用于确定字符串在屏幕上显示时需要的宽度。
 *
 * @param u8x8 指向u8x8显示结构体的指针
 * @param s 要计算的UTF-8字符串
 * @return 字符串中的字符数量
 */
uint8_t u8x8_GetUTF8Len(u8x8_t *u8x8, const char *s)
{
  uint16_t e;
  uint8_t cnt = 0;
  u8x8_utf8_init(u8x8);  /* 初始化UTF-8解码器 */
  for(;;)
  {
    e = u8x8_utf8_next(u8x8, *s);  /* 解码下一个字符 */
    if ( e == 0x0ffff )     /* 字符串结束 */
      break;
    s++;
    if ( e != 0x0fffe )     /* 跳过UTF-8中间字节 */
      cnt++;                /* 有效字符计数 */
  }
  return cnt;
}


