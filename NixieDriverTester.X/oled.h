#ifndef OLED_H
#define	OLED_H

#include <xc.h>

void InitDisplay(void);

void DrawCharacter(uint8_t row, uint8_t col, uint8_t code);

#endif	/* OLED_H */

