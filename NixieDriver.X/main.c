#include <xc.h>

#define _XTAL_FREQ (1000 * 1000ul) // 1 MHz

void InitPins()
{
    /**
    TRISx registers
    */
    TRISA = 0x3F;
    TRISB = 0x30;
    TRISC = 0xFF;

    /**
    ANSELx registers
    */
    ANSELA = 0x7;
    ANSELB = 0x30;
    ANSELC = 0xFF;
    
    // Clear the GPIO pins
    PORTA = 0;
    PORTB = 0;
    PORTC = 0;
    
    // Set PPS input pins
    SSP1CLKPPS = 0x5;
    SSP1DATPPS = 0x4;

    // Set PPS output pins
    RA5PPS = 0x07;
    RA4PPS = 0x08;
}

void InitInterrupts()
{
    PIR0bits.INTF = 0;
    INTCONbits.INTEDG = 1;

    // Enable global and peripheral interrupts
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 
}

void InitI2C()
{
    /* CKE disabled; SMP Standard Speed;  */
    SSP1STAT = 0xA4;
    /* SSPM 7 Bit Polling; CKP disabled; SSPEN disabled; SSPOV no_overflow; WCOL no_collision;  */
    SSP1CON1 |= 0x6;
    /* SEN enabled; RSEN disabled; PEN disabled; RCEN disabled; ACKEN disabled; ACKDT acknowledge; GCEN disabled;  */
    SSP1CON2 = 0x1;
    /* DHEN disabled; AHEN disabled; SBCDE disabled; SDAHT 100ns; BOEN disabled; SCIE disabled; PCIE disabled;  */
    SSP1CON3 = 0x0;
    /* SSPADD 32;  */
    SSP1ADD = 0x20;
    /* SSPMSK 254;  */
    SSP1MSK = 0xFE;

    /* Enable Interrupts */
    PIE1bits.SSP1IE = 1;

    SSP1CON1bits.SSPEN = 1;
}

volatile uint8_t gDataI2C = 0;
volatile uint8_t gNewDataI2C = 0;

void SendAckI2C()
{
    /* Send ACK */
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
}

void HandleI2C()
{
    // Clear the interrupt flag
    PIR1bits.SSP1IF = 0;
    
    if (!SSP1STATbits.D_nA)
    {
        // Handle address message
        
        uint8_t devNull = SSP1BUF;
        SendAckI2C();
    }
    else if ((!SSP1STATbits.R_nW) && (SSP1STATbits.BF))
    {
        // Handle data writes

        gDataI2C = SSP1BUF;
        gNewDataI2C = 1;
        
        SendAckI2C();
    }
    
    SSP1CON1bits.CKP = 1;
}

void __interrupt() ISR()
{
    if (PIR1bits.SSP1IF == 1) HandleI2C();
}

void main(void)
{
    InitInterrupts();
    InitPins();
    InitI2C();
    
    while(1)
    {
        if (gNewDataI2C)
        {
            uint8_t data = gDataI2C;
            gNewDataI2C = 0;
            
            RB6 = (data & 0x1) ? 1 : 0;
            RB7 = (data & 0x2) ? 1 : 0;
        }
        
        __delay_ms(10);
    }    
}
