/**
 * @file lv_port_disp.h
 * @brief LVGL显示接口移植文件，对接底层ILI9341 LCD驱动
 * @note 本文件作用：LVGL绘图引擎 ↔ 底层LCD驱动之间的桥梁
 *       LVGL把画面渲染到内存缓冲区，本文件负责把缓冲区像素数据刷到屏幕
 */
#include "lv_port_disp.h"
#include "lcd.h"                    /* 第 6 步写的底层LCD驱动 */

/* 1. LCD分辨率，必须和lcd.h中LCD_WIDTH/LCD_HEIGHT保持一致
 *    MY_DISP_HOR_RES：水平像素宽度
 *    MY_DISP_VER_RES：垂直像素高度
 */
#define MY_DISP_HOR_RES  480
#define MY_DISP_VER_RES  320

/* 2. 渲染缓冲区配置
 *    BUFFER_LINES：一次刷新多少行像素，数值越大刷新越快，占用内存越多
 *    BUFFER_SIZE：单个缓冲区像素总数 = 一行像素 × 缓存行数
 *    使用双缓冲buf1、buf2：
 *      LVGL在填充其中一块缓冲区时，代码同步将另一块缓冲区的数据发送到LCD
 *      减少等待，提升界面流畅度，避免撕裂
 *    static修饰：缓冲区存放全局静态存储区，不占用栈空间（栈空间很小，大数组不能放栈里）
 */
#define BUFFER_LINES     10
#define BUFFER_SIZE      (MY_DISP_HOR_RES * BUFFER_LINES)
static lv_color_t buf1[BUFFER_SIZE];    // 显示缓冲区1
static lv_color_t buf2[BUFFER_SIZE];    // 显示缓冲区2

/**
 * @brief LVGL显示刷新回调函数 disp_flush
 * @param disp_drv 显示驱动实例指针
 * @param area 需要刷新的屏幕区域结构体，包含x1,y1,x2,y2矩形坐标
 * @param color_p 像素颜色缓冲区首地址，LVGL已经渲染好的像素数组
 * @retval 无
 * @note LVGL内部渲染完成一块区域后，自动调用此回调，由用户实现像素发送硬件操作
 */
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    /* (1) 设置LCD显存写入窗口
     * 通知ILI9341：接下来SPI写入的像素，填充到area指定矩形区域(x1,y1)~(x2,y2)
     */
    LCD_SetWindow(area->x1, area->y1, area->x2, area->y2);

    /* (2) 计算本次需要发送的字节数（每个像素2字节） */
    uint32_t pixel_count = (area->x2 - area->x1 + 1) * (area->y2 - area->y1 + 1);

    /* LV_COLOR_16_SWAP=1: draw buffer内存中字节顺序已是SPI正确的（高字节在前）
     * 直接整块发送，避免逐像素CS/DC开关，速度提升数十倍
     */
    LCD_CS(0);
    LCD_DC(1);
    HAL_SPI_Transmit(&hspi1, (uint8_t *)color_p, pixel_count * 2, 100);
    LCD_CS(1);

    /* (3) 关键！通知LVGL：当前区域像素全部发送完毕，可以继续渲染下一帧
     * 如果缺少这个函数，LVGL会一直阻塞等待刷新完成，程序卡死无画面
     */
    lv_disp_flush_ready(disp_drv);
}

/**
 * @brief LVGL显示驱动初始化函数
 * @note 在main中调用，注册缓冲区、刷新回调，完成LVGL显示底层对接
 */
void lv_port_disp_init(void)
{
    /* (1) 静态变量，仅在第一次调用时分配内存，程序生命周期内只初始化一次 */
    static lv_disp_draw_buf_t disp_buf;      // LVGL缓冲区管理结构体
    static lv_disp_drv_t disp_drv;       // LVGL显示驱动结构体

    /* (2) 初始化显示缓冲区
     * 参数：缓冲区管理结构体，双缓冲buf1，双缓冲buf2，单个缓冲区最大像素数量
     */
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, BUFFER_SIZE);

    /* (3) 初始化驱动结构体，填充驱动默认参数 */
    lv_disp_drv_init(&disp_drv);

    /* (4) 配置驱动参数 */
    disp_drv.hor_res  = MY_DISP_HOR_RES;     // 设置屏幕水平分辨率
    disp_drv.ver_res  = MY_DISP_VER_RES;     // 设置屏幕垂直分辨率
    disp_drv.flush_cb = disp_flush;          // 绑定刷新回调函数，LVGL渲染完成后调用
    disp_drv.draw_buf   = &disp_buf;           // 绑定上面初始化好的双缓冲

    /* (5) 注册显示驱动到LVGL内核
     * 注册成功后LVGL就可以使用这个显示设备绘图
     */
    lv_disp_drv_register(&disp_drv);
}

