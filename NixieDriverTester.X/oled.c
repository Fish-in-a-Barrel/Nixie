#include <xc.h>

#include "oled.h"
#include "i2c.h"
#include "font8x5.h"

#define I2C_ADDRESS 0x3C

#define WIDTH 128
#define HEIGHT 32

#define OP_CLEAR 0
#define OP_CHARACTER 1

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
    uint8_t x = ((context->count - 1) % 128) / 10;
    uint8_t y = (context->count - 1) / 256;
    
    context->data = (0 == context->count) ? 0x40 : (x + y == 0 ? 0xFF : 0x00);
    return context->count <= (HEIGHT * WIDTH) / 8;
}

void Clear(void)
{
    static struct WriteCallbackContext context;
    
    SendAddressBounds(0, 0xFF, 0, WIDTH - 1);
    
    I2C_WriteWithCallback(I2C_ADDRESS, &ClearCallback, &context);
}

 
void InitDisplay(void)
{
    const uint8_t init1[] =
    {
        0x00,           // Start command sequence
        0xAE,           // Display off (10.1.12)
        0xD5, 0x80,     // Set display clock divider (10.1.16)
        0xA8, 0x1F      // Set multiplex ratio (10.1.11)
    };
    
    I2C_Write(I2C_ADDRESS, init1, sizeof(init1));
    
    const uint8_t init2[] =
    {
        0x00,           // Start command sequence
        0xD3, 0x00,     // Set display offset: 0 (10.1.15)
        0x40 | 0x00,    // Set display start line (10.1.6)
        0x8D, 0x14      // Set charge pump (2.1 of the application note)
    };
    
    I2C_Write(I2C_ADDRESS, init2, sizeof(init2));
    
    const uint8_t init3[] =
    {
        0x00,           // Start command sequence
        0x20, 0x00,     // Set memory mode: horizontal addressing (10.1.3)
        0xA0 | 0x00,    // Set segment remapping: 0 (10.1.8)
        0xC8            // Set COM output scan direction: normal (10.1.14)
    };
    
    I2C_Write(I2C_ADDRESS, init3, sizeof(init3));
    
    const uint8_t init4[] =
    {
        0x00,           // Start command sequence
        0xDA, 0x02,     // Set pin configuration: normal (10.1.18)
        0x81 | 0xA0     // Set contrast: 0 (10.1.7)
    };
    
    I2C_Write(I2C_ADDRESS, init4, sizeof(init4));
    
    const uint8_t init5[] =
    {
        0x00,           // Start command sequence
        0xA4,           // Render w/ RAM contents (10.1.9)
        0xA6,           // Set pixel state: normal (10.1.10)
        0x2E,           // Deactivate scrolling (10.2.3)
        0xAF            // Display on
    };
    
    I2C_Write(I2C_ADDRESS, init5, sizeof(init5));
    
    Clear();
}

uint8_t DrawCharacterCallback(struct WriteCallbackContext* context)
{
    if (0 == context->count)
    {
        context->data = 0x40;
        return 1;
    }
    else if (context->count <= FONT_WIDTH)
    {
        context->data = font8x5[context->id][context->count - 1];
        return 1;
    }
    else if (context->count == FONT_WIDTH + 1)
    {
        context->data = 0;
        return 1;
    }
    
    return 0;
}

void DrawCharacter(uint8_t row, uint8_t col, uint8_t code)
{
    static struct WriteCallbackContext context;
    context.id = code;
    
    SendAddressBounds(row, row + 1, col * 6, (col + 1) * 6);

    I2C_WriteWithCallback(I2C_ADDRESS, &DrawCharacterCallback, &context);
}