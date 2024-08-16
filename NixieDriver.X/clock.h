/* 
 * File:   clock.h
 * Author: aaron
 *
 * Created on November 11, 2023, 2:59 PM
 */

#ifndef CLOCK_H
#define	CLOCK_H

#define SPEED 16

#define _XTAL_FREQ (SPEED * 1000 * 1000ul)

void InitClock(void);

#endif	/* CLOCK_H */

