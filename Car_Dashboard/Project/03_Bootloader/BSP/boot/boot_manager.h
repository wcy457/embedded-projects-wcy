#ifndef __BOOT_MANAGER_H
#define __BOOT_MANAGER_H

#include "main.h"


/* ===== Flash 分区地址定义（STM32F103 大容量）===== */
#define INTER_FLASH_PARAM_ADDR   0x08006000u   /* 参数区起始地址，存放OTA标记、版本、校验和 */
#define INTER_FLASH_APP_ADDR     0x08008000u   /* APP应用分区起始地址，Bootloader跳转到这里运行APP */
#define INTER_FLASH_APP_END      0x08080000u   /* APP分区结束地址（边界，不包含该地址） */

/**
 * @brief  Flash参数区结构体，存储OTA相关配置信息
 * @note   #pragma pack(1) 1字节对齐，避免编译器填充多余内存，保证存储到Flash的二进制布局固定
 */
#pragma pack(1)
typedef struct {
    uint8_t  magic[4];         /* 偏移0：魔数标记 固定0xAA,0xBB,0xCC,0xDD，用来判断参数区是否有效 */
    uint32_t ota_bin_version;  /* 偏移4：固件版本号，用于版本比对 */
    uint8_t  ota_flag;         /* 偏移8：OTA升级标记，1=等待升级，0=正常运行 */
    uint8_t  checksum;         /* 偏移9：校验和，前9个字节累加的低8位，校验数据是否损坏 */
    uint8_t  format[2];        /* 偏移10：预留字节，后续扩展使用 */
} flash_cfg_param_t;
#pragma pack()  /* 取消1字节对齐，恢复默认对齐规则 */

/* 魔数宏定义，用于识别合法参数区 */
#define FLASH_CFG_MAGIC_0  0xAA
#define FLASH_CFG_MAGIC_1  0xBB
#define FLASH_CFG_MAGIC_2  0xCC
#define FLASH_CFG_MAGIC_3  0xDD

/* 参数区接口 */
uint8_t inter_flash_cfg_get_ota_flag(void);             /* 返回 flag；0xFF=参数区无效 */
void    inter_flash_cfg_set_app_update_flag(uint8_t flag);

/* 跳转接口: 检查通过并跳走(不返回)；APP 非法返回 1 */
uint8_t boot_check_stack2jump_app(uint32_t app_addr);
void    boot_jump_to_app(uint32_t app_addr);


#endif