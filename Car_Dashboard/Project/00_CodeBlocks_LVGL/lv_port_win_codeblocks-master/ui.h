#ifndef __UI_H__
#define __UI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl/lvgl.h"   /* PC 上靠 Include Path "." 找到 lvgl\lvgl.h; Keil 里第 9 步配路径 */

/*========== 6 个移植来的图片数组（定义在 images\*.c，这里只是声明） ==========*/
LV_IMG_DECLARE(ui_img_942215904);        /* 表盘背景 233x233 */
LV_IMG_DECLARE(ui_img_1601502596);       /* 指针     22x86  */
LV_IMG_DECLARE(ui_img_light_png);        /* 大灯     32x32  */
LV_IMG_DECLARE(ui_img_turn_light_png);   /* 转向     32x32  */
LV_IMG_DECLARE(ui_img_temp_gray_png);    /* 水温     32x32  */
LV_IMG_DECLARE(ui_img_safety_belt_png);  /* 安全带   32x32  */

/*========== 自己写的界面接口 ==========*/
void ui_init(void);     /* 建整个仪表盘界面 */

/* 6 个数据刷新接口（第 4 步在 ui.c 里实现） */
void ui_set_rpm_angle(uint32_t angle);
void ui_set_water_temp_color(uint32_t color);
void ui_set_safty_belt_color(uint32_t v);
void ui_set_light_color(uint32_t v);
void ui_set_turn_light_color(uint32_t v);
void ui_set_date_time_value(uint8_t *date_str, uint8_t *time_str);

#ifdef __cplusplus
}
#endif
#endif /* __UI_H__ */
