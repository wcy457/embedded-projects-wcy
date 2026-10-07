#ifndef __INTER_FLASHIF_H
#define __INTER_FLASHIF_H

#include "main.h"

#define FLASH_FAGE_SIZE  0x800u

uint8_t inter_flashif_erase_page(uint32_t addr);
uint8_t inter_flashif_write_page(uint32_t addr, uint32_t *buf, uint32_t len);
void inter_flashif_read(uint32_t addr, uint8_t *buf, uint32_t len);

#endif