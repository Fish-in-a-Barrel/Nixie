#include <pic16f15213.h>

#include "clock.h"

void InitClock(void)
{
#if SPEED == 2
    OSCFRQbits.FRQ = 0x1;
#elif SPEED == 4
    OSCFRQbits.FRQ = 0x2;
#elif SPEED == 8
    OSCFRQbits.FRQ = 0x3;
#elif SPEED == 16
    OSCFRQbits.FRQ = 0x4;
#elif SPEED == 32
    OSCFRQbits.FRQ = 0x5;
#else
    OSCFRQbits.FRQ = 0x0;
#endif
}
