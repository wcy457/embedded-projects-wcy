#ifndef __TIMER_H
#define __TIMER_H

extern volatile uint32_t System_tick_ms;

void Timer_Init(void);
uint32_t Time_Get_System_tick_ms(void);

#endif
