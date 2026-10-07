
/**
 * @file main
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <unistd.h>

#include "lvgl/lvgl.h"
#include "lv_demos/src/lv_demo_widgets/lv_demo_widgets.h"
#include "lv_drivers/win32drv/win32drv.h"
#include "lv_demos/src/lv_demo_base/lv_demo_base.h"

#include <windows.h>
#include "my_gui.h"
#include <time.h>


/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void hal_init(void);
static int tick_thread(void *data);

static uint32_t rpm = 0;/* ת�� 0~100 */

/**********************
 *  STATIC VARIABLES
 **********************/

 static void data_simulate(void)
 {
    /* ---- ÿ 300ms ��һ�γ������� ---- */
    static uint32_t last = 0;
    if (lv_tick_get() - last < 300) return;
    last = lv_tick_get();

    rpm = (rpm + 7) % 101;                     /* 0~100 ѭ������ */
    ui_set_rpm_angle(rpm);                     /* ָ�����ת */
    ui_set_water_temp_color(rpm > 80 ? 1 : 0); /* ��ת��ģ��ˮ�±��� */
    ui_set_safty_belt_color((rpm / 30) % 2);   /* ÿ 30 ��һ��ϵ/δϵ */
    ui_set_light_color(rpm > 50 ? 1 : 0);      /* ����̿���� */
    ui_set_turn_light_color((rpm / 20) % 2);   /* ת��������� */

    /* ---- ÿ 1s ˢ����ʱ�� (PC: ϵͳʱ��; STM32: ���� RTC) ---- */
    static uint32_t last_t = 0;
    if (lv_tick_get() - last_t < 1000) return;
    last_t = lv_tick_get();
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date[20], tim[20];
    sprintf(date, "%d-%d-%d", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
    sprintf(tim, "%02d:%02d", tm->tm_hour, tm->tm_min);
    ui_set_date_time_value((uint8_t *)date, (uint8_t *)tim);
 }

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR szCmdLine, int nCmdShow)
{
    /*Initialize LittlevGL*/
    lv_init();

    /*Initialize the HAL for LittlevGL*/
    lv_win32_init(hInstance, SW_SHOWNORMAL, 800, 480, NULL);

    /*Output prompt information to the console, you can also use printf() to print directly*/
    LV_LOG_USER("LVGL initialization completed!");

    /*Run the demo*/
   //lv_demo_widgets();
   my_gui();

    while(!lv_win32_quit_signal) {
        /* Periodically call the lv_task handler.
         * It could be done in a timer interrupt or an OS task too.*/
        lv_task_handler();
        usleep(10000);       /*Just to let the system breath*/
        data_simulate();        /* ����һ��: ģ������ˢ���Ǳ��� */
    }
    return 0;
}
