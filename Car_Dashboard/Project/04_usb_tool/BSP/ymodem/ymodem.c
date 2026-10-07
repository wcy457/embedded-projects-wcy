#include "ymodem.h"
#include "iso_tp.h"
#include "canif.h"
#include <string.h>

extern UART_HandleTypeDef huart2;  /* Ymodem serial link (PC side) */

/* Ymodem protocol control characters */
#define YM_SOH        0x01u   /* 128-byte data packet */
#define YM_STX        0x02u   /* 1024-byte data packet */
#define YM_EOT        0x04u   /* end of transmission */
#define YM_ACK        0x06u
#define YM_NAK        0x15u
#define YM_CA         0x18u   /* cancel, must be sent/seen twice */
#define YM_CRC16      'C'     /* request CRC16 mode */

#define YM_MAX_ERRORS 10u
#define YM_PKT_BUF    (PACKET_SIZE * 8 + 5u)   /* 1024 data + seq + ~seq + crc2 */

/* Frame layout in packet_data[]:
 *   [0] SOH/STX, [1] seq, [2] ~seq, [3 .. 3+dlen-1] data,
 *   [3+dlen] crc_hi, [4+dlen] crc_lo
 */

static uint8_t flash_buf[FLASH_PAGE_SIZE];   /* 2KB page accumulator */
static uint8_t packet_data[YM_PKT_BUF];      /* MUST be static: stack is 4KB only */

static void ym_putc(uint8_t c)
{
    HAL_UART_Transmit(&huart2, &c, 1, 100);
}

/* CRC16-CCITT (poly 0x1021, init 0), the standard Ymodem CRC */
static uint16_t ym_crc16(const uint8_t *p, uint16_t len)
{
    uint16_t crc = 0;
    uint16_t i;
    uint8_t  j;

    for (i = 0; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)p[i] << 8);
        for (j = 0; j < 8; j++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc = (uint16_t)(crc << 1);
            }
        }
    }
    return crc;
}

/**
  * @brief  Receive one Ymodem frame with full validation.
  * @param  dlen: output, data length (128/1024); 0 means EOT
  * @param  timeout: timeout for the first byte (ms)
  * @retval 0 ok, 1 error/timeout (NAK the sender), 2 double-CA abort
  */
static uint8_t ym_recv_packet(uint16_t *dlen, uint32_t timeout)
{
    uint8_t  ch;
    uint16_t data_len;
    uint16_t i;
    uint16_t crc_rx;

    *dlen = 0;

    if (HAL_UART_Receive(&huart2, &ch, 1, timeout) != HAL_OK) {
        return 1;
    }
    packet_data[0] = ch;

    if (ch == YM_EOT) {
        return 0;                       /* *dlen = 0 marks EOT */
    }
    if (ch == YM_CA) {
        /* abort is valid only as two consecutive CA bytes */
        if (HAL_UART_Receive(&huart2, &ch, 1, 100) == HAL_OK && ch == YM_CA) {
            return 2;
        }
        return 1;
    }
    if (ch == YM_SOH) {
        data_len = PACKET_SIZE;         /* 128 */
    } else if (ch == YM_STX) {
        data_len = PACKET_SIZE * 8;     /* 1024 */
    } else {
        return 1;
    }

    /* seq + ~seq + data + crc_hi + crc_lo = data_len + 4 bytes */
    for (i = 1; i < data_len + 4u; i++) {
        if (HAL_UART_Receive(&huart2, &packet_data[i], 1, 1000) != HAL_OK) {
            return 1;
        }
    }

    /* sequence complement check */
    if ((uint8_t)(packet_data[1] ^ packet_data[2]) != 0xFFu) {
        return 1;
    }
    /* CRC16 check (big-endian CRC field) */
    crc_rx = (uint16_t)(((uint16_t)packet_data[data_len + 3] << 8) |
                         packet_data[data_len + 4]);
    if (ym_crc16(&packet_data[3], data_len) != crc_rx) {
        return 1;
    }

    *dlen = data_len;
    return 0;
}

/**
  * @brief Ymodem receive session: PC --UART2--> 2KB page buffer
  *        --ISO-TP/CAN--> Bootloader.
  * @retval 0 success, -1 error/timeout, -2 cancelled by sender
  * @note  Block number 0 is the file header only while the session has NOT
  *        started (in_transfer gate); after that, seq wraps 255->0 naturally
  *        and wrapped packets stay ordinary data packets.
  */
int32_t Ymodem_Receive(void)
{
    uint16_t pkt_len = 0;
    uint32_t offset = 0;
    uint16_t buf_idx = 0;
    uint16_t pkt_expect = 0;     /* next Ymodem block number (0 = header) */
    uint8_t  in_transfer = 0;
    uint8_t  eot_once = 0;
    uint8_t  errors = 0;

    /* invite the sender in CRC16 mode */
    ym_putc(YM_CRC16);

    while (1) {
        uint8_t r = ym_recv_packet(&pkt_len, 5000);

        if (r == 2) {
            ym_putc(YM_ACK);
            return -2;
        }
        if (r != 0) {
            /* framing error / timeout: re-request, give up after N errors */
            if (++errors > YM_MAX_ERRORS) {
                ym_putc(YM_CA);
                ym_putc(YM_CA);
                return -1;
            }
            ym_putc(in_transfer ? YM_NAK : YM_CRC16);
            continue;
        }

        /* ---------------- EOT: standard double-EOT handshake ---------------- */
        if (pkt_len == 0) {
            if (!eot_once) {
                eot_once = 1;
                ym_putc(YM_NAK);       /* first EOT: refuse, sender repeats it */
                continue;
            }
            /* Flush the last partial page BEFORE ACKing the second EOT:
             * ISO-TP takes tens of ms, and the PC sends the empty header only
             * after seeing this ACK, so no UART overflow can happen meanwhile. */
            if (buf_idx > 0) {
                while ((buf_idx & 0x03u) != 0u) {
                    flash_buf[buf_idx++] = 0xFF;
                }
                if (iso_tp_send_data(offset, flash_buf, buf_idx) != 0) {
                    return -1;
                }
            }

            ym_putc(YM_ACK);           /* second EOT: accept */

            /* batch session terminator: sender emits one empty SOH header */
            ym_putc(YM_CRC16);
            if (ym_recv_packet(&pkt_len, 1000) == 0 &&
                pkt_len == PACKET_SIZE && packet_data[1] == 0u) {
                ym_putc(YM_ACK);
            }

            /* tell Bootloader the whole firmware is finished */
            {
                uint8_t end_msg[8] = {0xBE, 0xAD, 0xBE, 0xEF, 0x02, 0x00, 0x00, 0x00};
                Can_Send_Msg(CAN_ID_CALL_OTA, 8, end_msg);
            }
            return 0;
        }

        /* ---------------- header / data packets ---------------- */
        {
            uint8_t seq = packet_data[1];

            if (seq != (uint8_t)pkt_expect) {
                /* duplicate or out-of-order: NAK, sender repeats this packet */
                if (++errors > YM_MAX_ERRORS) {
                    ym_putc(YM_CA);
                    ym_putc(YM_CA);
                    return -1;
                }
                ym_putc(YM_NAK);
                continue;
            }

            /* file header (block 0) is valid ONLY before transfer starts */
            if (!in_transfer && seq == 0u) {
                in_transfer = 1;
                pkt_expect = 1;
                errors = 0;
                ym_putc(YM_ACK);
                ym_putc(YM_CRC16);
                continue;
            }
            if (!in_transfer) {
                ym_putc(YM_NAK);       /* data before any header: refuse */
                continue;
            }

            /* ordinary data packet */
            {
                uint16_t i;
                errors = 0;
                for (i = 0; i < pkt_len && buf_idx < FLASH_PAGE_SIZE; i++) {
                    flash_buf[buf_idx++] = packet_data[3 + i];
                }

                /* page full: hand one 2KB page to ISO-TP, then ACK the packet */
                if (buf_idx >= FLASH_PAGE_SIZE) {
                    if (iso_tp_send_data(offset, flash_buf, FLASH_PAGE_SIZE) != 0) {
                        return -1;
                    }
                    offset += FLASH_PAGE_SIZE;
                    buf_idx = 0;
                }
            }

            pkt_expect = (uint16_t)(pkt_expect + 1u);   /* wraps at 256 */
            ym_putc(YM_ACK);
        }
    }
}
