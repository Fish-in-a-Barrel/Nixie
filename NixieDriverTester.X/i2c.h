#ifndef I2C_H
#define	I2C_H

#include <xc.h>

struct WriteCallbackContext
{
    uint8_t id;
    uint16_t count;
    
    uint8_t data;
};

typedef uint8_t (WriteCallback)(struct WriteCallbackContext* context);

void I2C_Host_Init(void);

void I2C_Write(uint8_t address, const void* data, uint8_t len);

void I2C_WriteWithCallback(uint8_t address, WriteCallback* callback, struct WriteCallbackContext* context);

void I2C_WriteRead(uint8_t address, const void* writeData, uint8_t writeLen, void* readData, uint8_t readLen);

void I2C_HandleInterrupt(void);

#endif	/* I2C_H */

