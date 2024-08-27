#ifndef CLOCK_H
#define	CLOCK_H

///
/// The clock speed in MHz.
#define SPEED 32

// Needed for __delay_xx macros, etc.
#define _XTAL_FREQ (SPEED * 1000 * 1000ul)

///
/// Initializes the MCU oscillator.
void InitClock(void);

#endif	/* CLOCK_H */

