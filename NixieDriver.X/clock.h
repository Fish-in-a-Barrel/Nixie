#ifndef CLOCK_H
#define	CLOCK_H

// Clock speed in MHz. Powers of 2, from 1 to 32 are valid.
#define SPEED 16

// Needed for library functions like __delay
#define _XTAL_FREQ (SPEED * 1000 * 1000ul)

void InitClock(void);

#endif	/* CLOCK_H */

