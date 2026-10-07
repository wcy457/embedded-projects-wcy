#include "my_gui.h"
#include "lvgl.h"
#include "ui.h"

void my_gui(void)
{
    ui_init();

    ui_set_rpm_angle(50);          /* 指针应指 125°, 停在表盘中部 */
    ui_set_safty_belt_color(1);    /* 安全带图标变红 */
    ui_set_water_temp_color(1);    /* 水温图标变红 */
    ui_set_light_color(1);         /* 大灯变黄 */
    ui_set_turn_light_color(1);    /* 转向图标变红 */
    ui_set_date_time_value((uint8_t *)"2026-9-13", (uint8_t *)"20:30");
}


