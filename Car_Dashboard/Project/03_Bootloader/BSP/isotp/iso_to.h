#ifndef __ISO_TP_H
#define __ISO_TP_H
#include "main.h"

/* ISO-TP 帧类型定义，取CAN报文第一个字节高4bit */
#define ISO_TP_SF  0x00   /* 单帧 Single Frame: 短数据，一帧CAN报文完成传输 */
#define ISO_TP_FF  0x10   /* 首帧 First Frame: 长数据分包传输的第一个包，携带总长度 */
#define ISO_TP_CF  0x20   /* 连续帧 Consecutive Frame: 首帧之后的后续数据包 */
#define ISO_TP_FC  0x30   /* 流控帧 Flow Control: 接收方发给发送方，控制发送速率 */



void     iso_tp_init(void);                                 // ISO-TP协议栈初始化，清空状态和缓存
void     iso_tp_server(void);                               // ISO-TP服务轮询函数，放到主循环调用
uint8_t  iso_tp_is_ota_finished(void);                      // 查询OTA固件传输是否完成
void     iso_tp_on_receive(uint8_t *data, uint16_t len);    // 接收完整数据后的回调函数，在main.c中实现
#endif
