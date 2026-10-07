#include "ui.h"

static lv_obj_t *img_dash;      // 仪表盘背景图片对象句柄
static lv_obj_t *img_needle;    // 转速指针图片对象句柄
static lv_obj_t *img_light;     // 大灯图标句柄
static lv_obj_t *img_turn;      // 转向灯图标句柄
static lv_obj_t *img_temp;       // 水温图标句柄
static lv_obj_t *img_belt;      // 安全带图标句柄
static lv_obj_t *lab_date;      // 日期文本标签句柄
static lv_obj_t *lab_time;      // 时间文本标签句柄

#define COLOR_RED     0xff, 0x00, 0x00
#define COLOR_WHITE   0xff, 0xff, 0xff
#define COLOR_YELLOW  0xff, 0xff, 0x00

void ui_init(void)
{
    lv_obj_t *scr = lv_scr_act();// 获取当前激活屏幕作为父容器
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);/* 禁止屏幕滚动 */
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0);// 设置屏幕背景色：黑色
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);// 背景不透明度：完全不透明

    /* 1. 表盘背景(最先创建, 在最底层)：后创建的控件默认在顶层 */
    img_dash = lv_img_create(scr);                          // 在scr屏幕上创建图片对象
    lv_img_set_src(img_dash, &ui_img_942215904);            // 设置图片数据源：表盘背景图片
    lv_obj_set_align(img_dash, LV_ALIGN_CENTER);             // 设置对齐基准：屏幕中心
    lv_obj_set_pos(img_dash, -86, -4);                       // 在对齐基准基础上偏移坐标(x,y)

    /* 2. 指针: 多两行旋转设置 */
    img_needle = lv_img_create(scr);                         // 创建指针图片对象
    lv_img_set_src(img_needle, &ui_img_1601502596);          // 设置图片数据源：转速指针图片
    lv_obj_set_align(img_needle, LV_ALIGN_CENTER);
    lv_obj_set_pos(img_needle, -86, 71);
    lv_img_set_pivot(img_needle, 10, -30);    /* 设置旋转轴心：图片左上角+(10,-30); 负 y 在图外, 正好落到表盘圆心 */
    lv_img_set_angle(img_needle, 2500);       /* 设置图片初始角度 2500 → 250.0°; LVGL角度单位是 0.1°，要乘以10 */

    /* 3~6. 一排 4 个指示灯(同一水平线 y=93, 灰色图标, 颜色后续通过setter函数动态修改) */
    // 大灯图标
    img_light = lv_img_create(scr);
    lv_img_set_src(img_light, &ui_img_light_png);
    lv_obj_set_align(img_light, LV_ALIGN_CENTER);
    lv_obj_set_pos(img_light, 4, 93);

    // 转向灯图标
    img_turn = lv_img_create(scr);
    lv_img_set_src(img_turn, &ui_img_turn_light_png);
    lv_obj_set_align(img_turn, LV_ALIGN_CENTER);
    lv_obj_set_pos(img_turn, 54, 93);

    // 水温图标
    img_temp = lv_img_create(scr);
    lv_img_set_src(img_temp, &ui_img_temp_gray_png);
    lv_obj_set_align(img_temp, LV_ALIGN_CENTER);
    lv_obj_set_pos(img_temp, 104, 93);

    // 安全带图标
    img_belt = lv_img_create(scr);
    lv_img_set_src(img_belt, &ui_img_safety_belt_png);
    lv_obj_set_align(img_belt, LV_ALIGN_CENTER);
    lv_obj_set_pos(img_belt, 154, 93);

    /* 7. 日期: 普通 label文本控件 */
    lab_date = lv_label_create(scr);                                // 创建文本标签
    lv_label_set_text(lab_date, "2026-9-15");                        // 设置初始显示文字
    lv_obj_set_align(lab_date, LV_ALIGN_CENTER);
    lv_obj_set_pos(lab_date, 125, -68);
    lv_obj_set_style_text_color(lab_date, lv_color_hex(0xFFFFFF), 0); //文字颜色白色
    lv_obj_set_style_text_font(lab_date, &lv_font_montserrat_24, 0);  //字体Montserrat，字号24

    /* 8. 时间: 40 号大字 */
    lab_time = lv_label_create(scr);
    lv_label_set_text(lab_time, "23:00");
    lv_obj_set_align(lab_time, LV_ALIGN_CENTER);
    lv_obj_set_pos(lab_time, 125, -34);
    lv_obj_set_style_text_color(lab_time, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(lab_time, &lv_font_montserrat_40, 0); //字体Montserrat，字号40
}

/**
 * @brief 设置转速指针角度，把输入0~100的转速值映射为LVGL图片旋转角度
 * @param angle 输入转速范围 0 ~ 100
 * @note 表盘总刻度范围是250°；LVGL角度单位：1 = 0.1°，所以250°对应2500
 */
void ui_set_rpm_angle(uint32_t angle)
{
    // 映射公式：输入0~100 → 输出0 ~ 2500（0.1°单位）
    lv_img_set_angle(img_needle, angle * 2500 / 100);
}

/**
 * @brief 设置水温图标颜色
 * @param color 控制标志：0=白色(熄灭)，非0=红色(告警点亮)
 * @note img_recolor：LVGL图片重新染色；recolor_opa=255代表染色完全不透明，才能覆盖原图灰度
 */
void ui_set_water_temp_color(uint32_t color)
{
    // 三目运算符：color等于0选白色，否则选红色
    lv_color_t c = (color == 0) ? lv_color_make(COLOR_WHITE)
                                : lv_color_make(COLOR_RED);
    lv_obj_set_style_img_recolor(img_temp, c, 0);          // 设置图片染色颜色
    lv_obj_set_style_img_recolor_opa(img_temp, 255, 0);     // 染色不透明度，255=完全染色
}

/**
 * @brief 设置大灯图标颜色，支持三状态
 * @param color 状态值：0=白色(关闭)  1=黄色(近光)  2=红色(故障)
 */
void ui_set_light_color(uint32_t color)
{
    // 嵌套三目判断，依次匹配状态
    lv_color_t c = (color == 2) ? lv_color_make(COLOR_RED)
               : (color == 1) ? lv_color_make(COLOR_YELLOW)
                              : lv_color_make(COLOR_WHITE);
    lv_obj_set_style_img_recolor(img_light, c, 0);
    lv_obj_set_style_img_recolor_opa(img_light, 255, 0);
}

/**
 * @brief 更新界面日期和时间文本标签
 * @param date_str 日期字符串指针，例如"2026-09-15"
 * @param time_str 时间字符串指针，例如"14:30"
 * @note 将uint8_t*强制转为const char*，适配lv_label_set_text函数参数类型
 */
void ui_set_date_time_value(uint8_t *date_str, uint8_t *time_str)
{
    lv_label_set_text(lab_date, (const char *)date_str);
    lv_label_set_text(lab_time, (const char *)time_str);
}

/**
 * @brief 设置安全带图标颜色
 * @param color 0=白色(正常，已系安全带)，非0=红色告警(未系安全带)
 */
void ui_set_safty_belt_color(uint32_t color)
{
    lv_color_t c = (color == 0) ? lv_color_make(COLOR_WHITE)
                                : lv_color_make(COLOR_RED);
    lv_obj_set_style_img_recolor(img_belt, c, 0);
    lv_obj_set_style_img_recolor_opa(img_belt, 255, 0);
}

/**
 * @brief 设置转向灯图标颜色
 * @param color 0=白色(关闭)，非0=红色(点亮闪烁)
 */
void ui_set_turn_light_color(uint32_t color)
{
    lv_color_t c = (color == 0) ? lv_color_make(COLOR_WHITE)
                                : lv_color_make(COLOR_RED);
    lv_obj_set_style_img_recolor(img_turn, c, 0);
    lv_obj_set_style_img_recolor_opa(img_turn, 255, 0);
}

