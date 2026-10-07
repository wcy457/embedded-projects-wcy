#ifndef __STATE_MACHINE_H
#define __STATE_MACHINE_H

#include <stdint.h>

/*
 * 自动模式状态机：
 *   待机(STANDBY) --开自动--> 最小运行(MIN_RUN, 60s) --检测到烹饪--> 自动调节(AUTO_ADJUST)
 *   最小运行 60s 内无烹饪 → 退回待机
 *   自动调节：事件结束后延迟 10s 退回待机
 */
void     auto_fsm_init(void);
void     auto_fsm_start(void);                       /* 进入自动模式时调用 */
void     auto_fsm_reset(void);                       /* 退出自动模式时调用 */
uint16_t auto_fsm_step(uint16_t fusion_pwm, uint8_t cooking_event);

#endif /* __STATE_MACHINE_H */
