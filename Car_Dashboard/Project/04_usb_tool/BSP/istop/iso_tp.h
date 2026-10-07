#ifndef __ISO_TP_H
#define __ISO_TP_H

#include "main.h"

void     iso_tp_init(void);
uint8_t  iso_tp_send_data(uint32_t offset, uint8_t *data, uint16_t len);

#endif
