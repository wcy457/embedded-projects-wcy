#include "inter_flashif.h"

/**
 * @brief 擦除addr所在的Flash页面（F103ZE每页2KB）
 * @param addr 待擦除Flash地址，HAL库内部会自动向下对齐到页首地址
 * @retval uint8_t 0：擦除成功；1：擦除失败
 */
uint8_t inter_flashif_erase_page(uint32_t addr)
{
    FLASH_EraseInitTypeDef erase;        // Flash擦除初始化结构体，配置擦除参数
    uint32_t page_error = 0;             // 保存擦除失败的页面编号

    HAL_FLASH_Unlock();                  // 解锁Flash，Flash默认上锁，写/擦除前必须解锁

    erase.TypeErase   = FLASH_TYPEERASE_PAGES; // 擦除模式：按页擦除
    erase.Banks       = FLASH_BANK_1;          // 选择BANK1，F103ZE的Flash都在BANK1
    erase.PageAddress = addr;                  // 待擦除页地址，HAL内部自动对齐到页首
    erase.NbPages     = 1;                     // 擦除页面数量：1页

    // 执行页擦除，返回值判断操作结果，page_error记录出错页
    if (HAL_FLASHEx_Erase(&erase, &page_error) != HAL_OK) {
        HAL_FLASH_Lock();                    // 擦除失败，关闭Flash保护，上锁
        return 1;                            // 返回1代表擦除失败
    }
    HAL_FLASH_Lock();                        // 擦除成功，Flash重新上锁
    return 0;
}

/**
 * @brief 以32位字(4字节)为单位写入Flash
 * @param addr 写入起始Flash地址，必须4字节对齐
 * @param buf  源数据缓冲区，uint32_t数组
 * @param len  写入长度，单位：字(1字=4字节)
 * @retval uint8_t 0：写入成功；1：写入失败
 * @note 调用此函数前，**必须预先擦除对应Flash页面**
 * @note Flash特性：bit只能1→0，不能0→1，所以写前擦成全0xFF
 */
uint8_t inter_flashif_write_page(uint32_t addr, uint32_t *buf, uint32_t len)
{
    uint32_t i;
    HAL_FLASH_Unlock();                          // Flash解锁，准备编程写入

    // 循环逐个32位字写入Flash
    for (i = 0; i < len; i++) {
        // 单次写入1个32位字，目标地址=基地址 + i*4
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                              addr + i * 4, buf[i]) != HAL_OK) {
            HAL_FLASH_Lock();                    // 写入出错，立刻上锁Flash
            return 1;                            // 返回1代表写入失败
        }
    }
    HAL_FLASH_Lock();                            // 全部写入完成，Flash上锁
    return 0;
}

/**
 * @brief 读取Flash指定区域的数据
 * @param addr Flash读取的起始地址
 * @param buf  接收数据的缓冲区指针
 * @param len  需要读取的字节个数
 * @note STM32 Flash属于内存映射外设，直接指针访问读取
 * @note volatile关键字：防止编译器优化，保证直接读取硬件地址
 */
void inter_flashif_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint32_t i;
    // 循环逐个字节读取Flash
    for (i = 0; i < len; i++) {
        // volatile 强制从原始地址读取，不使用寄存器缓存
        buf[i] = *(volatile uint8_t *)(addr + i);
    }
}

