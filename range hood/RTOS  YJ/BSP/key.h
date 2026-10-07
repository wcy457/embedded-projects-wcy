#ifndef __KEY_H
#define __KEY_H

#include "main.h"

/* 按键事件 */
typedef enum {
    KEY_EVT_NONE = 0,
    KEY_EVT_SHORT,      /* 短按 */
    KEY_EVT_LONG        /* 长按（≥1s） */
} key_evt_t;

typedef enum {
    KEY_1 = 0,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_NUM//将按键数量`KEY_NUM`放在枚举最后一项，自动得到按键总数，新增按键只需要在枚举列表中加一行，数组长度自动更新，**不需要手动修改宏定义数值，减少出错概率**。
} key_id_t;

void      key_init(void);
key_evt_t key_scan(key_id_t id);   /* 每 10ms 调用一次，返回本次事件 */

#endif /* __KEY_H */
