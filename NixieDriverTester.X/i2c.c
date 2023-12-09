#include "i2c.h"
#include "i2c_register_bits.h"
#include "clock.h"
#include "pps_outputs.h"

#define I2C_BAUD (400 * 1000ul) // 100 kHz

#define I2C_ADDRESS(address, rw) (uint8_t)((address << 1) | rw)

#define I2C_READ 1
#define I2C_WRITE 0

//
// Operation states
//

#define OP_IDLE 0

// Operations
#define OP_WRITE 1

// States
#define STATE_ERROR 0xFF
#define STATE_IDLE 0
#define STATE_WRITE_ADDRESS 2
#define STATE_WRITE_DATA 3

static struct
{
    uint8_t type;
    uint8_t state;
    
    uint8_t address;
    
    WriteCallback* callback;
    struct WriteCallbackContext* callbackContext;
    
    const uint8_t* writeBuffer;
    uint8_t writeBufferLen;
} operation;

void ClearOp()
{
    if (operation.state == STATE_ERROR) 
    {
        SSP1CON1bits.SSPEN = 0;
        SSP1CON1bits.SSPEN = 1;        
    }

    operation.type = OP_IDLE;
    operation.state = STATE_IDLE;
    operation.callback = NULL;
    operation.callbackContext = NULL;
    operation.writeBuffer = NULL;
    operation.writeBufferLen = 0;
}

uint8_t IsBusy()
{
    return operation.type != OP_IDLE || SSP1STATbits.S || SSP1STATbits.BF;
}

void I2C_Host_Init(void)
{
    // disable slew rate control for standard speed
    SSP1STAT |= 
              SSPxSTAT_SLEW_RATE_CTL_ENABLED // 100 kHz
            | SSPxSTAT_CKE_SMBUS_DISABLED;
    
    // enable I2C pins SCL and SDA for serial communication (§25.2.4)
    SSP1CON1 = SSPxCON1_SSMP_HOST;
    SSP1CON2 = 0;
    SSP1CON3 = SSPxCON3_PCIE_ENABLED | SSPxCON3_SCIE_ENABLED;
    
    // set the baud rate (§25.3)
    SSP1ADD = _XTAL_FREQ / (4 * I2C_BAUD + 1);
    
    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
    
    // Do not use ClearOp or function duplication will occur since this is non-interrupt code.
    operation.type = OP_IDLE;
}

uint8_t Start()
{
    SSP1CON2bits.SEN = 1;
    return STATE_WRITE_ADDRESS;
}

uint8_t Stop()
{
    SSP1CON2bits.PEN = 1;
    ClearOp();
    
    return STATE_IDLE;
}

uint8_t WriteAddress()
{
    SSP1BUF = I2C_ADDRESS(operation.address, I2C_WRITE);
    return STATE_WRITE_DATA;
}

uint8_t WriteData()
{
    if (operation.callback && operation.callback(operation.callbackContext))
    {
        SSP1BUF = operation.callbackContext->data;
        ++operation.callbackContext->count;
        
        return STATE_WRITE_DATA;
    }
    else if (operation.writeBuffer && operation.writeBufferLen)
    {
        SSP1BUF = *(operation.writeBuffer++);
        --operation.writeBufferLen;

        return STATE_WRITE_DATA;
    }
    
    return Stop();
}

void ExecuteStateMachine()
{   
    if (SSP1CON2bits.ACKSTAT) operation.state = STATE_ERROR;
    
    switch (operation.state)
    {
        case STATE_WRITE_ADDRESS:
            operation.state = WriteAddress();
            break;
        case STATE_WRITE_DATA:
            operation.state = WriteData();
            break;

        default:
            ClearOp();
            break;
    }
}

void I2C_Write(uint8_t address, const void* data, uint8_t len)
{
    if (IsBusy()) return;
    
    operation.type = OP_WRITE;
    operation.address = address;
    operation.writeBuffer = data;
    operation.writeBufferLen = len;
    
    operation.callback = NULL;
    operation.callbackContext = NULL;    
    
    operation.state = STATE_WRITE_ADDRESS;
    Start();
    
    while (IsBusy());
}

void I2C_WriteWithCallback(uint8_t address, WriteCallback* callback, struct WriteCallbackContext* context)
{
    if (IsBusy()) return;
    
    operation.type = OP_WRITE;
    operation.address = address;
    operation.callback = callback;
    operation.callbackContext = context;
    operation.callbackContext->count = 0;    
    
    operation.writeBuffer = 0;
    operation.writeBufferLen = 0;
    
    operation.state = STATE_WRITE_ADDRESS;
    Start();
    
    while (IsBusy());
}

void I2C_HandleInterrupt(void)
{
    if (PIR1bits.SSP1IF)
    {
        PIR1bits.SSP1IF = 0;
        ExecuteStateMachine();
    }
    else if (PIR1bits.BCL1IF)
    {
        PIR1bits.BCL1IF = 0;
        operation.state = STATE_ERROR;
        ClearOp();
    }
}
