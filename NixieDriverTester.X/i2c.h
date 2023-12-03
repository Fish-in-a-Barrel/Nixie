#ifndef I2C_H
#define	I2C_H

#include <xc.h>

void I2C_Host_Init(void);

void I2C_Write(uint8_t address, const void* data, uint8_t len);

void I2C_WriteRead(uint8_t address, const void* writeData, uint8_t writeLen, void* readData, uint8_t readLen);

void I2C_HandleInterrupt(void);

#endif	/* I2C_H */

