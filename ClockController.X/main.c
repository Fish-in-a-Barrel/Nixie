#include "config_bits.h"
#include "i2c_register_bits.h"

#define _XTAL_FREQ (1000 * 1000) // 1 MHz
#define I2C_BAUD (100 * 1000) // 100 kHz

void I2C_Host_Init()
{
    // enable I2C pins SCL and SDA for serial communication
    SSP1CON1 = SSPxCON1_SSPEN_ENABLED | SSPxCON1_SSMP_HOST;
    SSP1CON2 = 0x00;
    
    // slew rate enables for high speed control
    SSP1STAT = SSPxSTAT_SLEW_RATE_CTL_ENABLED | SSPxSTAT_CKE_SMBUS_DISABLED;
    SSP1ADD = ((_XTAL_FREQ/4)/I2C_BAUD) - 1;
    
    // set RA4 & RA5 as digital inputs
    TRISA4 = 1;
    TRISA5 = 1;
}

void I2C_IsIdle()
{
  while (
          (SSP1STAT & SSPxSTAT_START_MASK) || 
          (SSP1CON2 & (SSPxCON2_ACKEN_ENABLED | SSPxCON2_RCEN_ENABLED)));
}

void I2C_Start()
{
    I2C_IsIdle();
    
     // initial start condition on SDA line
    SSP1CON2 |= SSPxCON2_SEN_ENABLED;
}

void I2C_Stop()
{
    I2C_IsIdle();
    
    // Initiate Stop condition on SDA and SCL pins
    SSP1CON2 |= SSPxCON2_PEN_ENABLED;
}

void I2C_Restart()
{
    I2C_IsIdle();

    // Initiate Repeated Start condition on SDA and SCL pins.
    SSP1CON2 |= SSPxCON2_RSEN_ENABLED;
}

void I2C_ACK(void)
{
    I2C_IsIdle();
    
    //Acknowledge Data bit
    SSP1CON2 &= ~SSPxCON2_ACKDT_ACK;
    
    // Acknowledge Sequence Enable bit
    SSP1CON2 |= ~SSPxCON2_ACKEN_ENABLED;
}

void I2C_NACK(void)
{
    I2C_IsIdle();
    
    //Acknowledge Data bit
    SSP1CON2 |= SSPxCON2_ACKDT_ACK;
    
    // Acknowledge Sequence Enable bit
    SSP1CON2 |= ~SSPxCON2_ACKEN_ENABLED;
}

// input parameter to this function can be a byte ( either address or data)
uint8_t I2C_Write(uint8_t data)
{
    I2C_IsIdle();
    
    SSP1BUF = data;
    while (SSP1STAT & SSPxSTAT_RW_HOST_TX_ACTIVE) {}
    
    return ACKSTAT;
}

void main(void)
{
    I2C_Host_Init();
    
    uint8_t counter = 0;
    
    while (1)
    {
        I2C_Write(0x50); // address
        I2C_Write(counter); // data
        
        counter = (counter + 1) % 10;
        
        __delay_ms(1000);
    }
}
