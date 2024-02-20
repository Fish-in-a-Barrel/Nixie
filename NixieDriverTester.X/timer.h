#ifndef TIMER_H
#define	TIMER_H

#include <xc.h>
#include "clock.h"

#define TMR2_POST 10
#define TMR2_FREQ (64 * 1000ul)
#define TICK_FREQ (TMR2_FREQ / TMR2_POST)
#define TMR2_RESET ((_XTAL_FREQ / 4) / TMR2_FREQ)

extern uint32_t gTickCount;

void InitTimer(void);

void TimerInterruptHandler(void);

#endif	/* TIMER_H */

