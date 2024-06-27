#ifndef OLED_H
#define	OLED_H

#include <xc.h>

void SetupDisplay(void);

void OLED_Clear(void);

void DrawCharacter(uint8_t row, uint8_t col, uint8_t code);

void OLED_DrawCharacterInverted(uint8_t row, uint8_t col, uint8_t code);

void DrawString(uint8_t row, uint8_t col, const char* str);

void OLED_DrawStringInverted(uint8_t row, uint8_t col, const char* str);

void OLED_DrawNumber8(uint8_t row, uint8_t col, uint8_t number, int8_t digitCount);

void OLED_DrawNumber16(uint8_t row, uint8_t col, uint16_t number, int8_t digitCount);

void InvertDisplay(uint8_t invert);

#endif	/* OLED_H */

