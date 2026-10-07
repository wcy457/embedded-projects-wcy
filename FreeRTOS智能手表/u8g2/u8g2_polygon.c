/**
 * @file u8g2_polygon.c
 * @brief u8g2库的多边形绘制功能实现文件
 *
 * 本文件实现了u8g2图形库中凸多边形的填充绘制功能。
 * 使用扫描线填充算法，通过左右两条边同时扫描来填充多边形内部。
 *
 * 算法概述：
 * 1. 找到多边形的最高点和最低点
 * 2. 从最低点开始，逐行扫描到最高点
 * 3. 维护左右两条活动边，每条边使用Bresenham算法计算交点
 * 4. 在每条扫描线上，从左交点到右交点画水平线
 *
 * @note 只支持凸多边形，不支持凹多边形
 * @note 最大支持点数由PG_MAX_POINTS定义（默认6）
 */


#include "u8g2.h"




/*===========================================*/
/* 局部数据结构定义 */

/** 多边形坐标使用的数据类型（16位有符号整数） */
typedef int16_t pg_word_t;

/**
 * @brief 多边形顶点结构体
 */
struct pg_point_struct
{
  pg_word_t x;    /**< 顶点的x坐标 */
  pg_word_t y;    /**< 顶点的y坐标 */
};

typedef struct _pg_struct pg_struct;	/* 前向声明 */

/**
 * @brief 多边形边结构体 - 用于扫描线填充算法
 *
 * 该结构体描述多边形的一条边，用于扫描线填充过程中
 * 计算每条扫描线与边的交点位置。
 */
struct pg_edge_struct
{
  pg_word_t x_direction;    /**< x方向: 1表示x2>x1（向右），-1表示向左 */
  pg_word_t height;         /**< 边的垂直高度（y2-y1） */
  pg_word_t current_x_offset;  /**< 每行x的整数增量（dx/height） */
  pg_word_t error_offset;   /**< 误差增量（dx % height） */

  /* --- 线段循环变量 --- */
  pg_word_t current_y;      /**< 当前扫描线的y坐标 */
  pg_word_t max_y;          /**< 边的最大y坐标 */
  pg_word_t current_x;      /**< 当前交点的x坐标 */
  pg_word_t error;          /**< Bresenham误差项 */

  /* --- 外循环变量 --- */
  uint8_t (*next_idx_fn)(pg_struct *pg, uint8_t i);  /**< 获取下一个顶点索引的函数指针 */
  uint8_t curr_idx;         /**< 当前顶点索引 */
};

/** 多边形的最大顶点数（可重定义，最大254） */
#define PG_MAX_POINTS 6

/** 左边索引 */
#define PG_LEFT 0
/** 右边索引 */
#define PG_RIGHT 1


/**
 * @brief 多边形填充的主要数据结构
 *
 * 包含多边形的所有顶点信息和扫描线填充所需的边结构。
 */
struct _pg_struct
{
  struct pg_point_struct list[PG_MAX_POINTS];  /**< 多边形顶点列表 */
  uint8_t cnt;                     /**< 当前顶点数量 */
  uint8_t is_min_y_not_flat;       /**< 标志: 最低y处的边是否不平坦 */
  pg_word_t total_scan_line_cnt;   /**< 总扫描线数量 */
  struct pg_edge_struct pge[2];    /**< 左边和右边的绘制结构 */
};


/*===========================================*/
/* 以下函数不应内联（尽可能节省Flash ROM空间） */

#define PG_NOINLINE U8G2_NOINLINE  /* 禁止内联的属性宏 */

/* 函数前向声明 */
static uint8_t pge_Next(struct pg_edge_struct *pge) PG_NOINLINE;      /**< 边的步进函数 */
static uint8_t pg_inc(pg_struct *pg, uint8_t i) PG_NOINLINE;          /**< 顶点索引递增 */
static uint8_t pg_dec(pg_struct *pg, uint8_t i) PG_NOINLINE;          /**< 顶点索引递减 */
static void pg_expand_min_y(pg_struct *pg, pg_word_t min_y, uint8_t pge_idx) PG_NOINLINE;  /**< 扩展最小y点 */
static void pg_line_init(pg_struct * const pg, uint8_t pge_index) PG_NOINLINE;  /**< 初始化边 */

/*===========================================*/
/* 线段绘制算法（Bresenham变体） */

/**
 * @brief 边的步进函数 - 计算下一条扫描线与边的交点
 *
 * 使用Bresenham算法的变体来计算边与下一条扫描线的交点x坐标。
 * 该算法避免了浮点运算，只使用整数加法和比较。
 *
 * @param pge   边结构体指针
 * @return      1表示成功步进，0表示已到达边的终点
 */
static uint8_t pge_Next(struct pg_edge_struct *pge)
{
  if ( pge->current_y >= pge->max_y )  /* 检查是否到达边的终点 */
    return 0;

  /* 每行x增加整数部分 */
  pge->current_x += pge->current_x_offset;
  /* 累积误差 */
  pge->error += pge->error_offset;
  /* 误差超过阈值时，x需要额外步进 */
  if ( pge->error > 0 )
  {
    pge->current_x += pge->x_direction;  /* x方向步进 */
    pge->error -= pge->height;           /* 重置误差 */
  }

  pge->current_y++;  /* 移动到下一条扫描线 */
  return 1;
}

/**
 * @brief 初始化边结构体
 *
 * 根据边的两个端点初始化边结构体，为扫描线填充做准备。
 * 假设y2 > y1（边从上到下）。
 *
 * @param pge   边结构体指针
 * @param x1    起点x坐标
 * @param y1    起点y坐标
 * @param x2    终点x坐标
 * @param y2    终点y坐标
 */
static void pge_Init(struct pg_edge_struct *pge, pg_word_t x1, pg_word_t y1, pg_word_t x2, pg_word_t y2)
{
  pg_word_t dx = x2 - x1;  /* x方向增量 */
  pg_word_t width;          /* dx的绝对值 */

  pge->height = y2 - y1;   /* 边的垂直高度 */
  pge->max_y = y2;          /* 边的最大y坐标 */
  pge->current_y = y1;      /* 起始y坐标 */
  pge->current_x = x1;      /* 起始x坐标 */

  if ( dx >= 0 )            /* 边向右倾斜 */
  {
    pge->x_direction = 1;   /* x方向为正 */
    width = dx;
    pge->error = 0;         /* 初始误差为0 */
  }
  else                      /* 边向左倾斜 */
  {
    pge->x_direction = -1;  /* x方向为负 */
    width = -dx;            /* 取绝对值 */
    pge->error = 1 - pge->height;  /* 初始误差 */
  }

  /* 计算每行x的整数增量和误差增量 */
  pge->current_x_offset = dx / pge->height;   /* 每行x的基本增量 */
  pge->error_offset = width % pge->height;     /* 误差累积量 */
}

/*===========================================*/
/* 凸多边形填充算法 */

/**
 * @brief 顶点索引递增（循环）
 *
 * @param pg    多边形结构体指针
 * @param i     当前索引
 * @return      下一个索引（循环到0）
 */
static uint8_t pg_inc(pg_struct *pg, uint8_t i)
{
    i++;
    if ( i >= pg->cnt )  /* 超出范围时循环到0 */
      i = 0;
    return i;
}

/**
 * @brief 顶点索引递减（循环）
 *
 * @param pg    多边形结构体指针
 * @param i     当前索引
 * @return      上一个索引（循环到末尾）
 */
static uint8_t pg_dec(pg_struct *pg, uint8_t i)
{
    i--;
    if ( i >= pg->cnt )  /* 下溢时循环到末尾 */
      i = pg->cnt-1;
    return i;
}

/**
 * @brief 扩展最小y点 - 寻找同一y值的最远顶点
 *
 * 当多个顶点具有相同的最小y值时，找到最左边或最右边的顶点。
 *
 * @param pg        多边形结构体指针
 * @param min_y     最小y值
 * @param pge_idx   边索引（PG_LEFT或PG_RIGHT）
 */
static void pg_expand_min_y(pg_struct *pg, pg_word_t min_y, uint8_t pge_idx)
{
  uint8_t i = pg->pge[pge_idx].curr_idx;
  for(;;)
  {
    i = pg->pge[pge_idx].next_idx_fn(pg, i);  /* 获取下一个顶点 */
    if ( pg->list[i].y != min_y )              /* y值不同则停止 */
      break;
    pg->pge[pge_idx].curr_idx = i;             /* 更新当前索引 */
  }
}

/**
 * @brief 准备多边形填充 - 查找极值点并初始化边结构
 *
 * 该函数为扫描线填充做准备工作：
 * 1. 设置左右边的索引遍历函数
 * 2. 查找多边形的最高点和最低点
 * 3. 计算总扫描线数
 * 4. 初始化左右边的起始位置
 *
 * @param pg    多边形结构体指针
 * @return      1表示准备成功，0表示多边形高度为0（无需绘制）
 */
static uint8_t pg_prepare(pg_struct *pg)
{
  pg_word_t max_y;
  pg_word_t min_y;
  uint8_t i;

  /* 设置右边使用递增索引，左边使用递减索引 */
  pg->pge[PG_RIGHT].next_idx_fn = pg_inc;
  pg->pge[PG_LEFT].next_idx_fn = pg_dec;

  /* 搜索最高点和最低点 */
  max_y = pg->list[0].y;
  min_y = pg->list[0].y;
  pg->pge[PG_LEFT].curr_idx = 0;
  for( i = 1; i < pg->cnt; i++ )
  {
    if ( max_y < pg->list[i].y )
    {
      max_y = pg->list[i].y;    /* 更新最高y值 */
    }
    if ( min_y > pg->list[i].y )
    {
      pg->pge[PG_LEFT].curr_idx = i;  /* 记录最低点索引 */
      min_y = pg->list[i].y;    /* 更新最低y值 */
    }
  }

  /* 计算总扫描线数（多边形高度） */
  pg->total_scan_line_cnt = max_y;
  pg->total_scan_line_cnt -= min_y;

  /* 多边形高度为0，无需绘制 */
  if ( pg->total_scan_line_cnt == 0 )
    return 0;

  /* 如果最低y处有多个顶点，找到最左边和最右边的 */
  pg->pge[PG_RIGHT].curr_idx = pg->pge[PG_LEFT].curr_idx;
  pg_expand_min_y(pg, min_y, PG_RIGHT);  /* 向右扩展 */
  pg_expand_min_y(pg, min_y, PG_LEFT);   /* 向左扩展 */

  /* 检查最低边是否平坦（取决于x值） */
  pg->is_min_y_not_flat = 1;
  if ( pg->list[pg->pge[PG_LEFT].curr_idx].x != pg->list[pg->pge[PG_RIGHT].curr_idx].x )
  {
    pg->is_min_y_not_flat = 0;  /* 最低边是平坦的 */
  }
  else
  {
    /* 最低边不平坦，减少一条扫描线 */
    pg->total_scan_line_cnt--;
    if ( pg->total_scan_line_cnt == 0 )
      return 0;
  }

  return 1;  /* 准备成功 */
}

/**
 * @brief 绘制一条水平扫描线
 *
 * 在当前扫描线上，从左边界到右边界绘制一条水平线。
 * 包含完整的边界检查和裁剪处理。
 *
 * @param pg    多边形结构体指针
 * @param u8g2  u8g2显示结构体指针
 */
static void pg_hline(pg_struct *pg, u8g2_t *u8g2)
{
  pg_word_t x1, x2, y;
  x1 = pg->pge[PG_LEFT].current_x;    /* 左边界x */
  x2 = pg->pge[PG_RIGHT].current_x;   /* 右边界x */
  y = pg->pge[PG_RIGHT].current_y;    /* 当前扫描线y */

  /* 检查y坐标是否在显示范围内 */
  if ( y < 0 )
    return;
  if ( y >= (pg_word_t)u8g2_GetDisplayHeight(u8g2) )
    return;

  if ( x1 < x2 )  /* 正常情况：左边界在右边界左边 */
  {
    /* 边界检查 */
    if ( x2 < 0 )
      return;
    if ( x1 >= (pg_word_t)u8g2_GetDisplayWidth(u8g2) )
      return;
    /* 裁剪到显示范围 */
    if ( x1 < 0 )
      x1 = 0;
    if ( x2 >= (pg_word_t)u8g2_GetDisplayWidth(u8g2) )
      x2 = u8g2_GetDisplayWidth(u8g2);
    /* 绘制水平线 */
    u8g2_DrawHLine(u8g2, x1, y, x2 - x1);
  }
  else  /* 异常情况：边界交换 */
  {
    /* 边界检查 */
    if ( x1 < 0 )
      return;
    if ( x2 >= (pg_word_t)u8g2_GetDisplayWidth(u8g2) )
      return;
    /* 裁剪到显示范围 */
    if ( x2 < 0 )
      x1 = 0;
    if ( x1 >= (pg_word_t)u8g2_GetDisplayWidth(u8g2) )
      x1 = u8g2_GetDisplayWidth(u8g2);
    /* 绘制水平线（注意x1和x2已交换） */
    u8g2_DrawHLine(u8g2, x2, y, x1 - x2);
  }
}

/**
 * @brief 初始化下一条边
 *
 * 当当前边扫描完成后，初始化下一条边继续扫描。
 * 从多边形顶点列表中获取下一条边的两个端点。
 *
 * @param pg         多边形结构体指针
 * @param pge_index  边索引（PG_LEFT或PG_RIGHT）
 */
static void pg_line_init(pg_struct * const pg, uint8_t pge_index)
{
  struct pg_edge_struct  *pge = pg->pge+pge_index;
  uint8_t idx;
  pg_word_t x1;
  pg_word_t y1;
  pg_word_t x2;
  pg_word_t y2;

  /* 获取当前边的起点 */
  idx = pge->curr_idx;
  y1 = pg->list[idx].y;
  x1 = pg->list[idx].x;
  /* 获取下一条边的终点 */
  idx = pge->next_idx_fn(pg, idx);
  y2 = pg->list[idx].y;
  x2 = pg->list[idx].x;
  pge->curr_idx = idx;  /* 更新当前索引 */

  /* 初始化边结构体 */
  pge_Init(pge, x1, y1, x2, y2);
}

/**
 * @brief 执行多边形填充的主循环
 *
 * 从最低点开始，逐行扫描到最高点，填充多边形内部。
 * 使用左右两条边同时扫描，每条边使用Bresenham算法。
 *
 * @param pg    多边形结构体指针
 * @param u8g2  u8g2显示结构体指针
 */
static void pg_exec(pg_struct *pg, u8g2_t *u8g2)
{
  pg_word_t i = pg->total_scan_line_cnt;  /* 剩余扫描线数 */

  /* 初始化左右两条边 */
  pg_line_init(pg, PG_LEFT);
  pg_line_init(pg, PG_RIGHT);

  /* 如果最低边不平坦，跳过第一条扫描线 */
  if ( pg->is_min_y_not_flat != 0 )
  {
    pge_Next(&(pg->pge[PG_LEFT]));
    pge_Next(&(pg->pge[PG_RIGHT]));
  }

  /* 主扫描循环 */
  do
  {
    pg_hline(pg, u8g2);  /* 绘制当前扫描线 */

    /* 左边步进，如果当前边结束则初始化下一条边 */
    while ( pge_Next(&(pg->pge[PG_LEFT])) == 0 )
    {
      pg_line_init(pg, PG_LEFT);
    }
    /* 右边步进，如果当前边结束则初始化下一条边 */
    while ( pge_Next(&(pg->pge[PG_RIGHT])) == 0 )
    {
      pg_line_init(pg, PG_RIGHT);
    }
    i--;
  } while( i > 0 );  /* 直到所有扫描线完成 */
}

/*===========================================*/
/* API 接口函数 */

/**
 * @brief 清空多边形顶点列表
 *
 * @param pg    多边形结构体指针
 */
static void pg_ClearPolygonXY(pg_struct *pg)
{
  pg->cnt = 0;  /* 重置顶点计数 */
}

/**
 * @brief 添加多边形顶点
 *
 * @param pg    多边形结构体指针
 * @param x     顶点x坐标
 * @param y     顶点y坐标
 */
static void pg_AddPolygonXY(pg_struct *pg, int16_t x, int16_t y)
{
  if ( pg->cnt < PG_MAX_POINTS )  /* 检查是否超出最大点数 */
  {
    pg->list[pg->cnt].x = x;     /* 存储x坐标 */
    pg->list[pg->cnt].y = y;     /* 存储y坐标 */
    pg->cnt++;                    /* 增加顶点计数 */
  }
}

/**
 * @brief 绘制多边形
 *
 * @param pg    多边形结构体指针
 * @param u8g2  u8g2显示结构体指针
 */
static void pg_DrawPolygon(pg_struct *pg, u8g2_t *u8g2)
{
  if ( pg_prepare(pg) == 0 )  /* 准备填充 */
    return;                   /* 多边形高度为0，无需绘制 */
  pg_exec(pg, u8g2);         /* 执行填充 */
}

/** 全局多边形结构体实例 */
pg_struct u8g2_pg;

/**
 * @brief 清空多边形顶点列表（用户API）
 */
void u8g2_ClearPolygonXY(void)
{
  pg_ClearPolygonXY(&u8g2_pg);
}

/**
 * @brief 添加多边形顶点（用户API）
 *
 * @param u8g2  u8g2显示结构体指针（未使用）
 * @param x     顶点x坐标
 * @param y     顶点y坐标
 */
void u8g2_AddPolygonXY(U8X8_UNUSED u8g2_t *u8g2, int16_t x, int16_t y)
{
  pg_AddPolygonXY(&u8g2_pg, x, y);
}

/**
 * @brief 绘制多边形（用户API）
 *
 * @param u8g2  u8g2显示结构体指针
 */
void u8g2_DrawPolygon(u8g2_t *u8g2)
{
  pg_DrawPolygon(&u8g2_pg, u8g2);
}

/**
 * @brief 绘制三角形
 *
 * 便捷函数，用于绘制填充的三角形。
 * 内部使用多边形绘制功能实现。
 *
 * @param u8g2  u8g2显示结构体指针
 * @param x0    第一个顶点的x坐标
 * @param y0    第一个顶点的y坐标
 * @param x1    第二个顶点的x坐标
 * @param y1    第二个顶点的y坐标
 * @param x2    第三个顶点的x坐标
 * @param y2    第三个顶点的y坐标
 */
void u8g2_DrawTriangle(u8g2_t *u8g2, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2)
{
  u8g2_ClearPolygonXY();                /* 清空顶点列表 */
  u8g2_AddPolygonXY(u8g2, x0, y0);     /* 添加第一个顶点 */
  u8g2_AddPolygonXY(u8g2, x1, y1);     /* 添加第二个顶点 */
  u8g2_AddPolygonXY(u8g2, x2, y2);     /* 添加第三个顶点 */
  u8g2_DrawPolygon(u8g2);              /* 绘制三角形 */
}

