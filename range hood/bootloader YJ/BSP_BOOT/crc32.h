#ifndef __CRC32_H
#define __CRC32_H

#include <stdint.h>

/* 增量式 CRC32，与 Python zlib.crc32 完全兼容（反射多项式 0xEDB88320） */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, uint32_t len);
uint32_t crc32_finalize(uint32_t crc);

/* 一次性计算 */
uint32_t crc32_calc(const uint8_t *data, uint32_t len);

#endif /* __CRC32_H */
