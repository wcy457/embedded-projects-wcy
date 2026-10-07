/**
 * @file u8g2_setup.c
 * @brief u8g2图形库的核心初始化和配置文件
 *
 * 本文件是u8g2库的核心模块之一，负责：
 * 1. u8g2对象的初始化和缓冲区配置
 * 2. 显示屏幕的旋转方向管理（支持0/90/180/270度旋转）
 * 3. 显示窗口裁剪(Clipping)功能
 * 4. 绘图坐标的维度计算和页面窗口更新
 * 5. 各种旋转方向下的线条绘制变换
 *
 * u8g2采用分页(Page)渲染机制，将显示缓冲区分为多个页面，
 * 每次渲染一个页面的内容，最终合并到整个屏幕上。
 * 这种机制可以在有限的RAM下驱动大尺寸显示屏。
 *
 * @note 屏幕旋转通过回调函数结构体(u8g2_cb_t)实现，
 *       每个旋转方向对应一组维度计算、窗口更新和线条绘制函数
 */

/*

  u8g2_setup.c

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
#include <string.h>
#include <assert.h>


/*============================================*/
/* 裁剪窗口(Clipping Window)功能模块 */
/* 裁剪窗口用于限制绘图区域，超出窗口范围的像素将不被绘制 */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT

/**
 * @brief 设置最大裁剪窗口（允许绘制整个屏幕）
 *
 * 将裁剪窗口设置为最大范围，即不限制绘图区域。
 * 裁剪窗口的左上角设为(0,0)，右下角设为最大值。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_SetMaxClipWindow(u8g2_t *u8g2)
{
  u8g2->clip_x0 = 0;                          /* 裁剪窗口左边界：0 */
  u8g2->clip_y0 = 0;                          /* 裁剪窗口上边界：0 */
  u8g2->clip_x1 = (u8g2_uint_t)~(u8g2_uint_t)0;  /* 裁剪窗口右边界：最大值(0xFFFF) */
  u8g2->clip_y1 = (u8g2_uint_t)~(u8g2_uint_t)0;  /* 裁剪窗口下边界：最大值(0xFFFF) */

  u8g2->cb->update_page_win(u8g2);  /* 更新页面窗口以应用新的裁剪设置 */
}

/**
 * @brief 设置自定义裁剪窗口
 *
 * 指定一个矩形区域作为绘图的有效范围。
 * 只有在该矩形区域内的像素才会被实际绘制到屏幕上。
 *
 * @param u8g2     u8g2设备结构体指针
 * @param clip_x0  裁剪窗口左边界x坐标
 * @param clip_y0  裁剪窗口上边界y坐标
 * @param clip_x1  裁剪窗口右边界x坐标（不含）
 * @param clip_y1  裁剪窗口下边界y坐标（不含）
 */
void u8g2_SetClipWindow(u8g2_t *u8g2, u8g2_uint_t clip_x0, u8g2_uint_t clip_y0, u8g2_uint_t clip_x1, u8g2_uint_t clip_y1 )
{
  u8g2->clip_x0 = clip_x0;  /* 设置裁剪窗口左边界 */
  u8g2->clip_y0 = clip_y0;  /* 设置裁剪窗口上边界 */
  u8g2->clip_x1 = clip_x1;  /* 设置裁剪窗口右边界 */
  u8g2->clip_y1 = clip_y1;  /* 设置裁剪窗口下边界 */
  u8g2->cb->update_page_win(u8g2);  /* 更新页面窗口以应用新的裁剪设置 */
}
#endif

/*============================================*/

/**
 * @brief 初始化u8g2对象的绘图缓冲区和基本参数
 *
 * 这是u8g2对象的核心初始化函数，在设置好底层显示驱动(u8x8)之后调用。
 * 该函数完成以下初始化工作：
 * 1. 设置绘图缓冲区指针和高度
 * 2. 配置底层像素绘制回调函数
 * 3. 初始化字体、颜色、透明度等绘图状态
 * 4. 设置屏幕旋转回调并计算显示维度
 * 5. 设置裁剪窗口（如果支持）
 *
 * @param u8g2           [输出] u8g2设备结构体指针
 * @param buf            显示缓冲区指针，用于存储待渲染的像素数据
 * @param tile_buf_height 缓冲区高度，以tile为单位（1 tile = 8像素高）
 *                       例如：8表示64像素高，4表示32像素高
 * @param ll_hvline_cb   底层水平/垂直线绘制回调函数，决定像素在缓冲区中的排列方式
 * @param u8g2_cb        屏幕旋转回调结构体，包含维度计算、窗口更新和线条绘制函数
 *
 * @note 该函数必须在u8g2_SetupDisplay()之后调用
 * @note 全屏缓冲模式下，tile_buf_height等于显示屏tile高度
 *       部分缓冲模式下，tile_buf_height小于显示屏tile高度，需要多次刷新
 */
void u8g2_SetupBuffer(u8g2_t *u8g2, uint8_t *buf, uint8_t tile_buf_height, u8g2_draw_ll_hvline_cb ll_hvline_cb, const u8g2_cb_t *u8g2_cb)
{
  u8g2->font = NULL;              /* 初始状态无字体，需后续调用u8g2_SetFont设置 */

  u8g2->ll_hvline = ll_hvline_cb; /* 设置底层像素绘制函数（决定像素排列方式） */

  u8g2->tile_buf_ptr = buf;       /* 设置显示缓冲区指针 */
  u8g2->tile_buf_height = tile_buf_height;  /* 设置缓冲区高度（以tile为单位） */

  u8g2->tile_curr_row = 0;        /* 当前渲染的tile行号，从0开始 */

  u8g2->font_decode.is_transparent = 0; /* 字体透明模式：0=不透明（实心背景） */
  u8g2->bitmap_transparency = 0;   /* 位图透明模式：0=不透明 */

  u8g2->font_height_mode = 0;     /* 字体高度计算模式：0=文本模式 */
  u8g2->draw_color = 1;           /* 绘图颜色：1=前景色（通常为白色/亮色） */
  u8g2->is_auto_page_clear = 1;   /* 自动清除页面：1=每页渲染前自动清除缓冲区 */

  u8g2->cb = u8g2_cb;             /* 设置旋转回调函数结构体 */
  u8g2->cb->update_dimension(u8g2); /* 根据旋转方向计算显示尺寸 */
#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
  u8g2_SetMaxClipWindow(u8g2);    /* 设置最大裁剪窗口并更新页面窗口 */
#else
  u8g2->cb->update_page_win(u8g2); /* 更新页面窗口（不支持裁剪时） */
#endif

  u8g2_SetFontPosBaseline(u8g2);  /* 设置字体位置为基线模式 */

#ifdef U8G2_WITH_FONT_ROTATION
  u8g2->font_decode.dir = 0;      /* 字体旋转方向：0=正常（不旋转） */
#endif
}

/**
 * @brief 设置显示屏幕的旋转方向
 *
 * 该函数用于动态更改显示屏的旋转方向。通常在初始化时设置，
 * 但也可以在运行时更改。更改后会自动更新显示维度和页面窗口。
 *
 * @param u8g2     u8g2设备结构体指针
 * @param u8g2_cb  旋转回调结构体指针，可选值：
 *                 - U8G2_R0: 不旋转（0度，正常方向）
 *                 - U8G2_R1: 顺时针旋转90度
 *                 - U8G2_R2: 旋转180度
 *                 - U8G2_R3: 逆时针旋转90度（或顺时针270度）
 *
 * @note 调用此函数会重新计算显示尺寸和页面窗口
 */
void u8g2_SetDisplayRotation(u8g2_t *u8g2, const u8g2_cb_t *u8g2_cb)
{
  u8g2->cb = u8g2_cb;                /* 设置新的旋转回调函数组 */
  u8g2->cb->update_dimension(u8g2);  /* 根据新旋转方向重新计算显示尺寸 */
  u8g2->cb->update_page_win(u8g2);   /* 更新页面窗口以匹配新尺寸 */
}

/*============================================*/

/**
 * @brief 向显示控制器发送格式化命令
 *
 * 使用类似printf的格式化语法向底层显示控制器发送命令。
 * 底层通过u8x8_cad_vsendf函数实现实际的命令发送。
 *
 * @param u8g2  u8g2设备结构体指针
 * @param fmt   格式化字符串，支持%d、%x等格式说明符
 * @param ...   可变参数列表
 */
void u8g2_SendF(u8g2_t * u8g2, const char *fmt, ...)
{
  va_list va;
  va_start(va, fmt);                           /* 初始化可变参数列表 */
  u8x8_cad_vsendf(u8g2_GetU8x8(u8g2), fmt, va); /* 发送格式化命令到显示控制器 */
  va_end(va);                                  /* 清理可变参数列表 */
}


/*============================================*/
/*
  维度更新函数：计算缓冲区的像素坐标范围
  计算以下变量：
    u8g2_uint_t buf_x0;  缓冲区左上角x坐标
    u8g2_uint_t buf_x1;  缓冲区右下角x坐标（不含）
    u8g2_uint_t buf_y0;  缓冲区左上角y坐标
    u8g2_uint_t buf_y1;  缓冲区右下角y坐标（不含）
*/

/**
 * @brief 通用的显示维度计算函数（所有旋转方向共用）
 *
 * 根据显示屏信息和当前缓冲区配置，计算以下关键参数：
 * - pixel_buf_height: 缓冲区像素高度
 * - pixel_buf_width: 缓冲区像素宽度
 * - pixel_curr_row: 当前渲染行的像素起始位置
 * - buf_y0/buf_y1: 缓冲区在屏幕上的y坐标范围
 * - width/height: 屏幕的逻辑宽高
 *
 * @param u8g2 u8g2设备结构体指针
 */
static void u8g2_update_dimension_common(u8g2_t *u8g2)
{
  const u8x8_display_info_t *display_info = u8g2_GetU8x8(u8g2)->display_info;
  u8g2_uint_t t;

  /* 计算缓冲区像素高度：tile数量 * 8像素/tile */
  t = u8g2->tile_buf_height;
  t *= 8;
  u8g2->pixel_buf_height = t;

  /* 计算缓冲区像素宽度：tile宽度 * 8像素/tile */
  t = display_info->tile_width;
#ifndef U8G2_16BIT
  /* 非16位模式下，限制最大宽度为31个tile（248像素），防止溢出 */
  if ( t >= 32 )
    t = 31;
#endif
  t *= 8;
  u8g2->pixel_buf_width = t;

  /* 计算当前渲染行的像素起始位置 */
  t = u8g2->tile_curr_row;
  t *= 8;
  u8g2->pixel_curr_row = t;

  /* 计算缓冲区实际高度，处理缓冲区大于剩余显示区域的情况 */
  t = u8g2->tile_buf_height;
  if ( t + u8g2->tile_curr_row > display_info->tile_height )
    t = display_info->tile_height - u8g2->tile_curr_row;  /* 截断到屏幕底部 */
  t *= 8;

  /* 设置缓冲区在屏幕上的y坐标范围 */
  u8g2->buf_y0 = u8g2->pixel_curr_row;   /* 缓冲区顶部y坐标 */
  u8g2->buf_y1 = u8g2->buf_y0;
  u8g2->buf_y1 += t;                      /* 缓冲区底部y坐标 */

  /* 设置屏幕逻辑尺寸 */
#ifdef U8G2_16BIT
  u8g2->width = display_info->pixel_width;   /* 16位模式：使用实际像素宽度 */
  u8g2->height = display_info->pixel_height;
#else
  /* 8位模式：宽度限制为最大240像素（8位坐标范围限制） */
  u8g2->width = 240;
  if ( display_info->pixel_width <= 240 )
    u8g2->width = display_info->pixel_width;
  u8g2->height = display_info->pixel_height;
#endif

}

/*==========================================================*/
/* 裁剪窗口应用函数 */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
/**
 * @brief 将用户绘图窗口与裁剪窗口求交集
 *
 * 该函数检查当前的用户绘图窗口是否与裁剪窗口有交集。
 * 如果有交集，则将用户窗口裁剪到裁剪窗口范围内。
 * 如果没有交集，则标记为无交集，后续绘图操作将被跳过。
 *
 * @param u8g2 u8g2设备结构体指针
 */
static void u8g2_apply_clip_window(u8g2_t *u8g2)
{
  /* 检查用户窗口与裁剪窗口是否有交集 */
  if ( u8g2_IsIntersection(u8g2, u8g2->clip_x0, u8g2->clip_y0, u8g2->clip_x1, u8g2->clip_y1) == 0 )
  {
    /* 无交集：标记为不可绘图 */
    u8g2->is_page_clip_window_intersection = 0;
  }
  else
  {
    /* 有交集：标记为可绘图，并将用户窗口限制在裁剪窗口范围内 */
    u8g2->is_page_clip_window_intersection = 1;

    /* 将用户窗口的左边界限制在裁剪窗口左边界之内 */
    if ( u8g2->user_x0 < u8g2->clip_x0 )
      u8g2->user_x0 = u8g2->clip_x0;
    /* 将用户窗口的右边界限制在裁剪窗口右边界之内 */
    if ( u8g2->user_x1 > u8g2->clip_x1 )
      u8g2->user_x1 = u8g2->clip_x1;
    /* 将用户窗口的上边界限制在裁剪窗口上边界之内 */
    if ( u8g2->user_y0 < u8g2->clip_y0 )
      u8g2->user_y0 = u8g2->clip_y0;
    /* 将用户窗口的下边界限制在裁剪窗口下边界之内 */
    if ( u8g2->user_y1 > u8g2->clip_y1 )
      u8g2->user_y1 = u8g2->clip_y1;
  }
}
#endif /* U8G2_WITH_CLIP_WINDOW_SUPPORT */

/*==========================================================*/
/* R0旋转方向（0度，正常方向）的维度和窗口更新函数 */

/**
 * @brief R0旋转方向的维度计算
 *
 * 0度旋转（正常方向）：直接使用通用维度计算函数，不做额外变换。
 * 屏幕坐标系：x向右，y向下。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_dimension_r0(u8g2_t *u8g2)
{
  u8g2_update_dimension_common(u8g2);
}

/**
 * @brief R0旋转方向的页面窗口更新
 *
 * 设置用户绘图窗口为整个屏幕宽度和当前缓冲区高度范围。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_page_win_r0(u8g2_t *u8g2)
{
  u8g2->user_x0 = 0;                /* 用户窗口左边界：屏幕最左边 */
  u8g2->user_x1 = u8g2->width;      /* 用户窗口右边界：屏幕宽度 */

  u8g2->user_y0 = u8g2->buf_y0;     /* 用户窗口上边界：当前缓冲区顶部 */
  u8g2->user_y1 = u8g2->buf_y1;     /* 用户窗口下边界：当前缓冲区底部 */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
  u8g2_apply_clip_window(u8g2);      /* 应用裁剪窗口限制 */
#endif
}

/*==========================================================*/
/* R1旋转方向（顺时针90度）的维度和窗口更新函数 */

/**
 * @brief R1旋转方向的维度计算
 *
 * 顺时针旋转90度：宽度和高度互换。
 * 原来的像素宽度变为逻辑高度，原来的像素高度变为逻辑宽度。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_dimension_r1(u8g2_t *u8g2)
{
  u8g2_update_dimension_common(u8g2);

  /* 旋转90度：宽高互换 */
  u8g2->height = u8g2_GetU8x8(u8g2)->display_info->pixel_width;   /* 新高度=原宽度 */
  u8g2->width = u8g2_GetU8x8(u8g2)->display_info->pixel_height;  /* 新宽度=原高度 */

}

/**
 * @brief R1旋转方向的页面窗口更新
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_page_win_r1(u8g2_t *u8g2)
{
  /* 旋转90度后，原来的y方向变为x方向 */
  u8g2->user_x0 = u8g2->buf_y0;     /* 用户窗口左边界 */
  u8g2->user_x1 = u8g2->buf_y1;     /* 用户窗口右边界 */

  u8g2->user_y0 = 0;                /* 用户窗口上边界 */
  u8g2->user_y1 = u8g2->height;     /* 用户窗口下边界 */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
  u8g2_apply_clip_window(u8g2);      /* 应用裁剪窗口限制 */
#endif
}

/*==========================================================*/
/* R2旋转方向（旋转180度）的维度和窗口更新函数 */

/**
 * @brief R2旋转方向的维度计算
 *
 * 旋转180度：宽高不变，但坐标原点移动到右下角。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_dimension_r2(u8g2_t *u8g2)
{
  u8g2_update_dimension_common(u8g2);
}

/**
 * @brief R2旋转方向的页面窗口更新
 *
 * 旋转180度后，y轴方向反转，需要对y坐标进行镜像变换。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_page_win_r2(u8g2_t *u8g2)
{
  u8g2->user_x0 = 0;
  u8g2->user_x1 = u8g2->width;

  /* 旋转180度：y轴镜像 */
  u8g2->user_y0 = 0;
  /* 处理高度不是8的倍数的情况，此时buf_y1可能大于屏幕高度 */
  if ( u8g2->height >= u8g2->buf_y1 )
    u8g2->user_y0 = u8g2->height - u8g2->buf_y1;  /* 计算镜像后的顶部位置 */
  u8g2->user_y1 = u8g2->height - u8g2->buf_y0;    /* 计算镜像后的底部位置 */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
  u8g2_apply_clip_window(u8g2);
#endif
}

/*==========================================================*/
/* R3旋转方向（逆时针90度/顺时针270度）的维度和窗口更新函数 */

/**
 * @brief R3旋转方向的维度计算
 *
 * 逆时针旋转90度：宽度和高度互换（与R1类似）。
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_dimension_r3(u8g2_t *u8g2)
{
  u8g2_update_dimension_common(u8g2);

  /* 旋转270度：宽高互换（与R1相同） */
  u8g2->height = u8g2_GetU8x8(u8g2)->display_info->pixel_width;
  u8g2->width = u8g2_GetU8x8(u8g2)->display_info->pixel_height;

}

/**
 * @brief R3旋转方向的页面窗口更新
 *
 * @param u8g2 u8g2设备结构体指针
 */
void u8g2_update_page_win_r3(u8g2_t *u8g2)
{
  /* 处理宽度不是8的倍数的情况 */
  u8g2->user_x0 = 0;
  if ( u8g2->width >= u8g2->buf_y1 )
    u8g2->user_x0 = u8g2->width - u8g2->buf_y1;
  u8g2->user_x1 = u8g2->width - u8g2->buf_y0;

  u8g2->user_y0 = 0;
  u8g2->user_y1 = u8g2->height;

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
  u8g2_apply_clip_window(u8g2);
#endif
}


/*============================================*/
/* 各旋转方向下的线条绘制变换函数 */
/* 这些函数将逻辑坐标转换为物理坐标后，调用u8g2_draw_hv_line_2dir绘制线条 */
/* dir参数：0=水平线，1=垂直线 */

/* 声明外部的双方向线条绘制函数 */
extern void u8g2_draw_hv_line_2dir(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir);

/**
 * @brief R0旋转方向（0度）的线条绘制
 *
 * 正常方向绘制，不需要坐标变换，直接调用底层绘制函数。
 *
 * @param u8g2  u8g2设备结构体指针
 * @param x     起始x坐标
 * @param y     起始y坐标
 * @param len   线条长度（像素）
 * @param dir   方向：0=水平（向右），1=垂直（向下）
 */
void u8g2_draw_l90_r0(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
#ifdef __unix
  assert( dir <= 1 );  /* 调试模式下检查方向参数有效性 */
#endif
  u8g2_draw_hv_line_2dir(u8g2, x, y, len, dir);  /* 直接绘制，无需坐标变换 */
}

/**
 * @brief 水平镜像（左右翻转）的线条绘制
 *
 * 将x坐标进行水平镜像变换后绘制线条。
 * 用于实现水平方向的显示镜像效果。
 *
 * @param u8g2  u8g2设备结构体指针
 * @param x     原始x坐标
 * @param y     y坐标
 * @param len   线条长度
 * @param dir   方向：0=水平，1=垂直
 */
void u8g2_draw_l90_mirrorr_r0(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  u8g2_uint_t xx;
  xx = u8g2->width;   /* 获取屏幕宽度 */
  xx -= x;            /* 镜像变换：新x = 屏幕宽度 - 原x */
  if ( (dir & 1) == 0 )
  {
    xx -= len;        /* 水平线：需要减去线条长度以保持方向一致 */
  }
  else
  {
    xx--;             /* 垂直线：减1进行边界对齐 */
  }
  u8g2_draw_hv_line_2dir(u8g2, xx, y, len, dir);  /* 使用变换后的坐标绘制 */
}

/**
 * @brief 垂直镜像（上下翻转）的线条绘制
 *
 * 将y坐标进行垂直镜像变换后绘制线条。
 * 用于实现垂直方向的显示镜像效果。
 *
 * @param u8g2  u8g2设备结构体指针
 * @param x     x坐标
 * @param y     原始y坐标
 * @param len   线条长度
 * @param dir   方向：0=水平，1=垂直
 */
void u8g2_draw_mirror_vertical_r0(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  u8g2_uint_t yy;
  yy = u8g2->height;  /* 获取屏幕高度 */
  yy -= y;            /* 镜像变换：新y = 屏幕高度 - 原y */
  if ( (dir & 1) == 1 )
  {
    yy -= len;        /* 垂直线：需要减去线条长度以保持方向一致 */
  }
  else
  {
    yy--;             /* 水平线：减1进行边界对齐 */
  }
  u8g2_draw_hv_line_2dir(u8g2, x, yy, len, dir);  /* 使用变换后的坐标绘制 */
}

/**
 * @brief R1旋转方向（顺时针90度）的线条绘制
 *
 * 将坐标系旋转90度后绘制线条。
 * 坐标变换：新x = 屏幕高度 - 原y - 1，新y = 原x
 * 方向变换：水平变垂直，垂直变水平
 *
 * @param u8g2  u8g2设备结构体指针
 * @param x     原始x坐标
 * @param y     原始y坐标
 * @param len   线条长度
 * @param dir   方向：0=水平，1=垂直
 */
/* dir = 0 or 1 */
void u8g2_draw_l90_r1(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  u8g2_uint_t xx, yy;

#ifdef __unix
  assert( dir <= 1 );
#endif

  yy = x;                    /* 旋转90度：新y = 原x */

  xx = u8g2->height;         /* 旋转90度：新x = 屏幕高度 - 原y - 1 */
  xx -= y;
  xx--;

  dir ++;                    /* 方向旋转：0->1(垂直)，1->2->0(水平) */
  if ( dir == 2 )
  {
    xx -= len;               /* 垂直线：需要调整起始位置 */
    xx++;
    dir = 0;                 /* 方向2等价于方向0（水平） */
  }

  u8g2_draw_hv_line_2dir(u8g2, xx, yy, len, dir);
}

/**
 * @brief R2旋转方向（旋转180度）的线条绘制
 *
 * 将坐标系旋转180度后绘制线条。
 * 坐标变换：新x = 屏幕宽度 - 原x，新y = 屏幕高度 - 原y
 *
 * @param u8g2  u8g2设备结构体指针
 * @param x     原始x坐标
 * @param y     原始y坐标
 * @param len   线条长度
 * @param dir   方向：0=水平，1=垂直
 */
void u8g2_draw_l90_r2(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  u8g2_uint_t xx, yy;

  /* 旋转180度：坐标翻转 */
  yy = u8g2->height;         /* 新y = 屏幕高度 - 原y */
  yy -= y;

  xx = u8g2->width;          /* 新x = 屏幕宽度 - 原x */
  xx -= x;

  if ( dir == 0 )
  {
    yy--;                    /* 水平线：调整y坐标偏移 */
    xx -= len;               /* 水平线：调整x起始位置 */
  }
  else if ( dir == 1 )
  {
    xx--;                    /* 垂直线：调整x坐标偏移 */
    yy -= len;               /* 垂直线：调整y起始位置 */
  }

  u8g2_draw_hv_line_2dir(u8g2, xx, yy, len, dir);
}

/**
 * @brief R3旋转方向（逆时针90度/顺时针270度）的线条绘制
 *
 * 将坐标系旋转270度后绘制线条。
 * 坐标变换：新x = 原y，新y = 屏幕宽度 - 原x
 * 方向变换：水平变垂直，垂直变水平
 *
 * @param u8g2  u8g2设备结构体指针
 * @param x     原始x坐标
 * @param y     原始y坐标
 * @param len   线条长度
 * @param dir   方向：0=水平，1=垂直
 */
void u8g2_draw_l90_r3(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
{
  u8g2_uint_t xx, yy;

  xx = y;                    /* 旋转270度：新x = 原y */

  yy = u8g2->width;          /* 旋转270度：新y = 屏幕宽度 - 原x */
  yy -= x;

  if ( dir == 0 )
  {
    yy--;                    /* 水平线变垂直线：调整坐标 */
    yy -= len;
    yy++;
    dir = 1;                 /* 水平线变垂直线 */
  }
  else
  {
    yy--;                    /* 垂直线变水平线：调整坐标 */
    dir = 0;                 /* 垂直线变水平线 */
  }

  u8g2_draw_hv_line_2dir(u8g2, xx, yy, len, dir);
}


/*============================================*/
/* 旋转回调函数结构体定义 */
/* 每个结构体包含三个函数指针：维度计算、页面窗口更新、线条绘制 */

/* 四个标准旋转方向的回调结构体 */
const u8g2_cb_t u8g2_cb_r0 = { u8g2_update_dimension_r0, u8g2_update_page_win_r0, u8g2_draw_l90_r0 };   /* 0度旋转 */
const u8g2_cb_t u8g2_cb_r1 = { u8g2_update_dimension_r1, u8g2_update_page_win_r1, u8g2_draw_l90_r1 };   /* 90度旋转 */
const u8g2_cb_t u8g2_cb_r2 = { u8g2_update_dimension_r2, u8g2_update_page_win_r2, u8g2_draw_l90_r2 };   /* 180度旋转 */
const u8g2_cb_t u8g2_cb_r3 = { u8g2_update_dimension_r3, u8g2_update_page_win_r3, u8g2_draw_l90_r3 };   /* 270度旋转 */

/* 镜像模式的回调结构体 */
const u8g2_cb_t u8g2_cb_mirror = { u8g2_update_dimension_r0, u8g2_update_page_win_r0, u8g2_draw_l90_mirrorr_r0 };          /* 水平镜像 */
const u8g2_cb_t u8g2_cb_mirror_vertical = { u8g2_update_dimension_r0, u8g2_update_page_win_r0, u8g2_draw_mirror_vertical_r0 }; /* 垂直镜像 */
  
/*============================================*/
/* 空设备(Null Device)设置函数 */
/* 空设备用于测试或调试，不连接实际显示硬件 */

/**
 * @brief 初始化空设备（无实际显示输出）
 *
 * 创建一个不连接实际显示硬件的u8g2对象。
 * 主要用途：
 * - 测试绘图逻辑而不需要实际硬件
 * - 性能测试和调试
 * - 作为占位符，后续替换为实际设备
 *
 * @param u8g2              u8g2设备结构体指针
 * @param rotation          屏幕旋转回调结构体
 * @param byte_cb           字节传输回调函数（空设备中不使用）
 * @param gpio_and_delay_cb GPIO和延时回调函数（空设备中不使用）
 *
 * @note 空设备使用最小的8字节缓冲区（1个tile）
 * @note u8x8_d_null_cb为空显示驱动，u8x8_cad_empty为空通信协议
 */
void u8g2_Setup_null(u8g2_t *u8g2, const u8g2_cb_t *rotation, u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb)
{
  static uint8_t buf[8];  /* 最小缓冲区：8字节（1个tile = 8x8像素） */
  /* 设置空显示驱动和空通信协议 */
  u8g2_SetupDisplay(u8g2, u8x8_d_null_cb, u8x8_cad_empty, byte_cb, gpio_and_delay_cb);
  /* 使用最小缓冲区初始化绘图环境 */
  u8g2_SetupBuffer(u8g2, buf, 1, u8g2_ll_hvline_vertical_top_lsb, rotation);
}

