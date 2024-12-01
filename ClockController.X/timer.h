#ifndef TIMER_H
#define	TIMER_H

#include <xc.h>
#include "clock.h"

#define TMR2_POST 2 // [1-16]
#define TMR2_FREQ (64 * 1000ul)
#define TMR2_RESET ((_XTAL_FREQ / 4) / TMR2_FREQ)

void InitTimer(void);

void TimerInterruptHandler(void);

#endif	/* TIMER_H */

