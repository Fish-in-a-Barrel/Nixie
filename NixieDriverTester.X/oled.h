#ifndef OLED_H
#define	OLED_H

#include <xc.h>

void SetupDisplay(void);

void DrawCharacter(uint8_t row, uint8_t col, uint8_t code);

void InvertDisplay(uint8_t invert);

#endif	/* OLED_H */

