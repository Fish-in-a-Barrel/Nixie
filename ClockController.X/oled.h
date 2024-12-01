#ifndef OLED_H
#define	OLED_H

#include <xc.h>

///
/// Initializes the OLED display.
void OLED_Init(void);

///
/// Turns the OLED display on.
void OLED_On(void);

///
/// Turns the OLED display off.
void OLED_Off(void);

/// Erases the OLED display.
///
/// @note This is pretty slow because the entire memory bank of the display has to be overwritten with zeros via I2C.
void OLED_Clear(void);

/// Draws a single character to the display.
///
/// @param row The row offset of the character.
/// @param col The column offset of the character.
/// @param ascii The ASCII code of the character to draw.
/// @param invert If 1, draws the character in black on a white background.
void OLED_DrawCharacter(uint8_t row, uint8_t col, uint8_t ascii, uint8_t invert);

/// Draws a string of characters to the display.
///
/// @param row The row offset of the first character.
/// @param col The column offset of first the character.
/// @param str The string of ASCII characters to draw.
/// @param invert If 1, draws the string in black on a white background.
void OLED_DrawString(uint8_t row, uint8_t col, const char* str, uint8_t invert);

/// Draws an 8-bit integer to the display.
///
/// @param row The row offset of the first character.
/// @param col The column offset of first the character.
/// @param number The value to draw.
/// @param digitCount The number of digits to draw.
void OLED_DrawNumber8(uint8_t row, uint8_t col, uint8_t number, int8_t digitCount);

/// Draws a 15-bit integer to the display.
///
/// @param row The row offset of the first character.
/// @param col The column offset of first the character.
/// @param number The value to draw.
/// @param digitCount The number of digits to draw.
void OLED_DrawNumber16(uint8_t row, uint8_t col, uint16_t number, int8_t digitCount);

/// Inverts/reverts the display.
///
/// @param invert 1 to invert, 0 for normal.
void OLED_InvertDisplay(uint8_t invert);

#endif	/* OLED_H */

