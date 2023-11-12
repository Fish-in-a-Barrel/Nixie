#include "i2c.h"
#include "i2c_register_bits.h"
#include "clock.h"
#include "pps_outputs.h"

#define I2C_BAUD (100 * 1000ul) // 100 kHz

#define I2C_ADDRESS(address, rw) (uint8_t)((address << 1) | rw)

//
// Operation states
//

#define OP_IDLE 0

// OP_WRITE
#define OP_WRITE 1
#define STATE_WRITE_START 1
#define STATE_WRITE_ADDRESS 2
#define STATE_WRITE_DATA 3

typedef uint8_t (*StateHandler)(void);

static struct
{
    uint8_t type;
    uint8_t state;
    uint8_t address;
    uint8_t* buffer;
    uint8_t bufferLen;
    
    StateHandler stateHandler;
} operation;

void ClearOp()
{
    operation.type = OP_IDLE;
    operation.state = 0;
    operation.buffer = NULL;
    operation.bufferLen = 0;
    
    operation.stateHandler = NULL;
}

uint8_t IsBusy()
{
    return operation.type != OP_IDLE || SSP1STATbits.S;
}

void I2C_Host_Init(void)
{
    // Remap the SDA/SLC pins to 4/5 (§18.2, Table 18-1 / §18.8.2)
    SSP1DATPPS = 4;
    SSP1CLKPPS = 5;
    RA4PPS = PPS_OUT_SDA1;
    RA5PPS = PPS_OUT_SCL1;
    
    // set RA4 & RA5 as digital inputs (§25.2.2.3)
    TRISA |= 0x30;
    
    // Clear the analog registers (§16.5)
    ANSELA = 0x00;

    // Clear the GPIO pins
    PORTA = 0;
    
    // disable slew rate control for standard speed
    SSP1STAT |= 
              SSPxSTAT_SLEW_RATE_CTL_ENABLED // 100 kHz
            | SSPxSTAT_CKE_SMBUS_DISABLED;
    
    // enable I2C pins SCL and SDA for serial communication (§25.2.4)
    SSP1CON1 = SSPxCON1_SSMP_HOST;
    SSP1CON2 = 0;
    SSP1CON3 = 0;
    
    // set the baud rate (§25.3)
    SSP1ADD = _XTAL_FREQ / (4 * I2C_BAUD + 1);
    
    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
    
    // Do not use ClearOp or function duplication will occur since this is non-interrupt code.
    operation.type = OP_IDLE;
}

void I2C_HandleInterrupt(void)
{
    operation.stateHandler();
    
    // Clear the interrupt flag
    PIR1bits.SSP1IF = 0;
}

void I2C_Start()
{
    SSP1CON2bits.SEN = 1;
}

void I2C_Stop()
{
    SSP1CON2bits.PEN = 1;
}

void I2C_Restart()
{
    SSP1CON2bits.SEN = 1;
}

void I2C_SendACK(void)
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
}

void I2C_SendNACK(void)
{
    SSP1CON2bits.ACKDT = 1;
    SSP1CON2bits.ACKEN = 1;
}

uint8_t WriteAddress()
{
    SSP1BUF = operation.address;
    return STATE_WRITE_ADDRESS;
}

uint8_t WriteData()
{
    if ((NULL == operation.buffer) || (0 == operation.bufferLen))
    {
        I2C_Stop();
        ClearOp();
    }
    else
    {
        --operation.bufferLen;
        SSP1BUF = *(operation.buffer++);
    }

    return STATE_WRITE_DATA;
}

uint8_t WriteHandler()
{
    switch (operation.state)
    {
        case STATE_WRITE_START:
            operation.state = WriteAddress();
            break;
        case STATE_WRITE_ADDRESS:
        case STATE_WRITE_DATA:
            operation.state = WriteData();
            break;
        default:
            ClearOp();
            break;
    }
}

uint8_t I2C_Write(uint8_t address, uint8_t direction, uint8_t* data, uint8_t len)
{
    if (IsBusy()) return 0;
    
    operation.type = OP_WRITE;
    operation.state = STATE_WRITE_START;
    operation.address = I2C_ADDRESS(address, direction);
    operation.buffer = data;
    operation.bufferLen = len;
    
    operation.stateHandler = WriteHandler;
    
    I2C_Start();
    
    return ACKSTAT;
}
