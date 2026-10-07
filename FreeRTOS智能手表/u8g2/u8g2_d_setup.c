/**
 * @file u8g2_d_setup.c
 * @brief u8g2库的显示设备初始化设置文件
 *
 * 本文件包含针对特定显示控制器(SSD1306)的初始化配置函数，
 * 以及STM32平台相关的延时回调函数。
 *
 * SSD1306是一款常用的OLED显示控制器，支持128x64像素分辨率，
 * 通过I2C接口与STM32微控制器通信。
 *
 * @note 本文件由u8g2项目的codebuild工具自动生成，但STM32延时函数为手动添加
 */

/* generated code, codebuild, u8g2 project */

#include "u8g2.h"
#include "cmsis_os.h"   /* FreeRTOS CMSIS-OS接口头文件，提供HAL_Delay等函数 */

/**
 * @brief 设置SSD1306 I2C 128x64 OLED显示屏（全屏缓冲模式）
 *
 * 该函数完成SSD1306 OLED显示屏的完整初始化，包括：
 * 1. 配置显示驱动（SSD1306驱动芯片）
 * 2. 配置通信协议（I2C快速模式）
 * 3. 分配全屏显示缓冲区（1024字节）
 * 4. 设置绘图回调函数和屏幕旋转方向
 *
 * @param u8g2         [输出] u8g2设备结构体指针，用于后续所有绘图操作
 * @param rotation     屏幕旋转回调结构体，可选值：
 *                     - U8G2_R0: 不旋转（正常方向）
 *                     - U8G2_R1: 顺时针旋转90度
 *                     - U8G2_R2: 旋转180度
 *                     - U8G2_R3: 逆时针旋转90度
 * @param byte_cb      I2C字节传输回调函数，负责实际的数据发送
 * @param gpio_and_delay_cb  GPIO和延时回调函数，负责引脚控制和时序延时
 *
 * @note "f"后缀表示全屏(Full)缓冲模式，占用1024字节RAM但刷新效果好
 * @note 适用于128x64像素的SSD1306 OLED屏幕，如常见的0.96寸OLED模块
 */
/* ssd1306 f */
void u8g2_Setup_ssd1306_i2c_128x64_noname_f(u8g2_t *u8g2, const u8g2_cb_t *rotation, u8x8_msg_cb byte_cb, u8x8_msg_cb gpio_and_delay_cb)
{
  uint8_t tile_buf_height;  /* 缓冲区高度，以tile为单位（1 tile = 8像素高） */
  uint8_t *buf;             /* 显示缓冲区指针 */

  /* 第一步：初始化底层显示驱动和通信协议 */
  /* u8x8_d_ssd1306_128x64_noname: SSD1306 128x64显示驱动函数 */
  /* u8x8_cad_ssd13xx_fast_i2c: SSD13xx系列I2C快速通信协议 */
  u8g2_SetupDisplay(u8g2, u8x8_d_ssd1306_128x64_noname, u8x8_cad_ssd13xx_fast_i2c, byte_cb, gpio_and_delay_cb);

  /* 第二步：分配1024字节的全屏显示缓冲区，返回缓冲区高度(8个tile) */
  buf = u8g2_m_16_8_f(&tile_buf_height);

  /* 第三步：配置u8g2的绘图缓冲区和旋转方向 */
  /* u8g2_ll_hvline_vertical_top_lsb: 垂直排列、顶部起始、LSB优先的像素排列方式 */
  u8g2_SetupBuffer(u8g2, buf, tile_buf_height, u8g2_ll_hvline_vertical_top_lsb, rotation);
}

/**
 * @brief STM32平台的GPIO和延时回调函数
 *
 * 该函数处理u8g2库发出的GPIO控制和延时请求。
 * u8g2库通过消息机制与硬件平台交互，此函数作为桥梁
 * 将u8g2的延时请求转换为STM32的HAL_Delay调用。
 *
 * @param u8x8     u8x8底层设备结构体指针（本函数中未使用）
 * @param msg      消息类型，决定需要执行的操作：
 *                 - U8X8_MSG_GPIO_AND_DELAY_INIT: GPIO和延时初始化（本函数为空操作）
 *                 - U8X8_MSG_DELAY_MILLI: 毫秒级延时
 * @param arg_int  消息参数，对于延时消息表示延时的毫秒数
 * @param arg_ptr  消息参数指针（本函数中未使用）
 *
 * @return 1 表示消息已成功处理，0 表示不支持的消息类型
 *
 * @note 此函数在STM32平台上使用HAL_Delay进行延时
 * @note 在RTOS环境中，HAL_Delay可能会触发任务调度，需注意实时性
 * @warning 如需更精确的延时，可考虑使用DWT或TIM定时器替代
 */
uint8_t u8g2_stm32_delay(U8X8_UNUSED u8x8_t *u8x8, U8X8_UNUSED uint8_t msg, U8X8_UNUSED uint8_t arg_int, U8X8_UNUSED void *arg_ptr)
{
	switch(msg){

	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		/* GPIO和延时初始化消息：此处不需要额外初始化 */
		/* GPIO通常在CubeMX或main函数中已完成初始化 */
	    break;

	case U8X8_MSG_DELAY_MILLI:
		/* 毫秒延时消息：调用STM32 HAL库的延时函数 */
		HAL_Delay(arg_int);  /* 使用HAL库延时，arg_int为延时毫秒数 */
	    break;

	default:
		/* 不支持的消息类型，返回0表示未处理 */
		return 0;
	}
	/* 返回1表示消息已成功处理 */
	return 1;
}
/* end of generated code */
