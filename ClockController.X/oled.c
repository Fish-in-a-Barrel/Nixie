#include <xc.h>

#include "oled.h"
#include "i2c.h"
#include "font8x5.h"

#define I2C_ADDRESS 0x3C

#define WIDTH 128
#define HEIGHT 32

static struct WriteCallbackContext gContext;

void SendAddressBounds(uint8_t rowStart, uint8_t rowEnd, uint8_t colStart, uint8_t colEnd)
{
    uint8_t command[] =
    {
        0x00,
        0x22, 0x00, 0x00,
        0x21, 0x00, 0x00
    };
    
    command[2] = rowStart;
    command[3] = rowEnd;
    command[5] = colStart;
    command[6] = colEnd;
    
    I2C_Write(I2C_ADDRESS, command, sizeof(command));
}

uint8_t ClearCallback(struct WriteCallbackContext* context)
{
    if (0 == context->count)
    {
        context->data = 0x40;
        return 1;
    }
    else if (context->count <= (HEIGHT * WIDTH) / 8)
    {
        context->data = 0;
        return 1;
    }
    
    gContext.id = 0xFF;
    return 0;
}

void OLED_Clear(void)
{
    SendAddressBounds(0, 0xFF, 0, WIDTH - 1);
    
    gContext.id = 0;
    
    I2C_WriteWithCallback(I2C_ADDRESS, &ClearCallback, &gContext);
}

 
void SetupDisplay(void)
{
    /*
     * If pins are on the left:
     * A1, C8
     * 
     * If pins are on the right:
     * A0, C0
     */
    
    const uint8_t initCmds[] =
    {
        0x00,           // Start command sequence
        0xAE,           // Display off (10.1.12)
        0xD5, 0x80,     // Set display clock divider (10.1.16)
        0xA8, 0x1F,     // Set multiplex ratio (10.1.11)
        0xD3, 0x00,     // Set display offset: 0 (10.1.15)
        0x40 | 0x00,    // Set display start line (10.1.6)
        0x8D, 0x14,     // Set charge pump (2.1 of the application note)
        0x20, 0x00,     // Set memory mode: horizontal addressing (10.1.3)
        0xA0,           // Set segment remapping: 1 (10.1.8) *
        0xC0,           // Set COM output scan direction: normal (10.1.14) *
        0xDA, 0x02,     // Set pin configuration: normal (10.1.18)
        0x81, 0xA0,    // Set contrast: 0 (10.1.7)
        0xA4,           // Render w/ RAM contents (10.1.9)
        0xA6,           // Set pixel state: normal (10.1.10)
        0x2E,           // Deactivate scrolling (10.2.3)
        0xAF            // Display on
    };
    
    I2C_Write(I2C_ADDRESS, initCmds, sizeof(initCmds));
    
    OLED_Clear();
}

void DrawCharacterImpl(uint8_t row, uint8_t col, uint8_t ascii, uint8_t invert)
{
    uint8_t buffer[] = { 0x40, 0, 0, 0, 0, 0, invert ? 0xFF : 0 };
    for (uint8_t i = 0; i < FONT_WIDTH; ++i) buffer[i + 1] = invert ? ~font8x5[ascii][i] : font8x5[ascii][i];
    
    SendAddressBounds(row, row + 1, col * 6, (col + 1) * 6);
    I2C_Write(I2C_ADDRESS, buffer, sizeof(buffer));
}

void DrawCharacter(uint8_t row, uint8_t col, uint8_t ascii)
{
    DrawCharacterImpl(row, col, ascii, 0);
}

void OLED_DrawCharacterInverted(uint8_t row, uint8_t col, uint8_t ascii)
{
    DrawCharacterImpl(row, col, ascii, 1);
}

void DrawString(uint8_t row, uint8_t col, const char* str)
{
    char* c = (char*)str;
    while (*c != 0) DrawCharacter(row, col++, *(c++));
}

void OLED_DrawStringInverted(uint8_t row, uint8_t col, const char* str)
{
    char* c = (char*)str;
    while (*c != 0) OLED_DrawCharacterInverted(row, col++, *(c++));
}

void OLED_DrawNumber8(uint8_t row, uint8_t col, uint8_t number, int8_t digitCount)
{
    while (digitCount > 0)
    {
        DrawCharacter(row, (uint8_t)(col + --digitCount), '0' + number % 10);
        number /= 10;
    }
}

void OLED_DrawNumber16(uint8_t row, uint8_t col, uint16_t number, int8_t digitCount)
{
    while (digitCount > 0)
    {
        DrawCharacter(row, (uint8_t)(col + --digitCount), '0' + number % 10);
        number /= 10;
    }
}

void InvertDisplay(uint8_t invert)
{
    uint8_t command[] = { 0x00, 0xA6 };
    if (invert) command[1] = 0xA7;
    
    I2C_Write(I2C_ADDRESS, command, sizeof(command));
}