#include "crc32.h"

/*
 * 反射式 CRC32（多项式 0xEDB88320），初值 0xFFFFFFFF，结果异或 0xFFFFFFFF。
 * 与 Python 的 zlib.crc32 输出一致，用于 Bootloader 校验。
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len)
{
    uint32_t i;
    int j;

    for (i = 0; i < len; i++) {
        crc ^= data[i];
        for (j = 0; j < 8; j++) {
            /* 等价于 (crc & 1) ? 0xFFFFFFFF : 0 */
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(0u - (crc & 1u)));
        }
    }
    return crc;
}

uint32_t crc32_finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFu;
}

uint32_t crc32_calc(const uint8_t *data, uint32_t len)
{
    return crc32_finalize(crc32_update(0xFFFFFFFFu, data, len));
}
