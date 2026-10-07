#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include <stdint.h>

/*==================== Flash分区定义 STM32F103ZET6 512KB ====================*/
/* Bootloader占用：0x08000000 ~ 0x08003FFF (16KB) */
#define APP_BASE_ADDR         0x08004000UL     /* APP程序Flash起始地址 */
#define APP_MAX_SIZE          0x7C000UL        /* APP最大可用大小 496KB */
#define F1_FLASH_PAGE_SIZE    0x800UL          /* F1高容量芯片，每页2KB */

/*==================== 串口接收配置 ====================*/
#define BOOT_CHUNK_SIZE       128U             /* DMA分片接收缓冲区大小 */

/*==================== 按键升级引脚定义 PB0 ====================*/
#define BOOT_KEY_GPIO_PORT    GPIOB
#define BOOT_KEY_GPIO_PIN     GPIO_PIN_0

uint8_t boot_jump_to_app(void); /* 0=invalid app, never returns on success */


void boot_run_update(void);

uint8_t boot_is_key_pressed(void);


#endif
