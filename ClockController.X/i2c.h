#ifndef I2C_H
#define	I2C_H

#include <xc.h>

#define I2C_READ 1
#define I2C_WRITE 0

void I2C_Host_Init(void);

void I2C_IsIdle();

void I2C_Start();

void I2C_Stop();

void I2C_Restart();

void I2C_SendACK(void);

void I2C_SendNACK(void);

uint8_t I2C_Write(uint8_t address, uint8_t direction, uint8_t* data, uint8_t len);

void I2C_HandleInterrupt(void);

void I2C_HandleCollisionInterrupt(void);

#endif	/* I2C_H */

