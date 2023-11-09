//
// Section references are to the "PIC16F15213/14/23/24/43/44 Low Pin Count Microcontrollers" datasheet.
// Document DS40002195D
// https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/PIC16F15213-14-23-24-43-44-Microcontroller-Data-Sheet-40002195.pdf
//

#include <xc.h>

#define _XTAL_FREQ (1000 * 1000ul) // 1 MHz

#define CATHODE_0_PIN RB4
#define CATHODE_1_PIN RB5
#define CATHODE_2_PIN RB6
#define CATHODE_3_PIN RB7
#define CATHODE_4_PIN RC0
#define CATHODE_5_PIN RC1
#define CATHODE_6_PIN RC2
#define CATHODE_7_PIN RC3
#define CATHODE_8_PIN RC4
#define CATHODE_9_PIN RC5

uint8_t gAddressI2c = 0x10;

void InitPins()
{
    // I2C pins must be configured as inputs (§25.2.2.3)
    TRISA = 0x30;
    TRISB = 0x00;
    TRISC = 0x00;

    // Clear the analog registers (§16.5)
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    
    // Clear the GPIO pins
    PORTA = 0;
    PORTB = 0;
    PORTC = 0;
    
    // Set PPS input pins (§18.2, Table 18-1)
    SSP1CLKPPS = 0x5;
    SSP1DATPPS = 0x4;

    // Set PPS output pins (§18.8.2)
    RA5PPS = 0x07;
    RA4PPS = 0x08;
}

void InitInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 
}

void InitI2C()
{
    // Not relevant to clients (§25.4.4)
    SSP1STAT = 0x00;
    
    // 7-bit addressing client (§25.4.5)
    SSP1CON1 = 0x06;
    
    // Enable clock stretching (§25.4.6)
    SSP1CON2 = 0x01;
    
    // No start/stop interrupts (§25.4.7)
    SSP1CON3 = 0x00;
    
    // Set the client address and enable the full address mask (§25.4.2, §25.4.3)
    SSP1ADD = gAddressI2c << 1;
    SSP1MSK = 0xFE;

    // Enable Interrupts
    PIE1bits.SSP1IE = 1;

    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
}

volatile uint8_t gDataI2C = 0;
volatile uint8_t gNewDataI2C = 0;

void SendAckI2C()
{
    SSP1CON2bits.ACKDT = 0;
    SSP1CON2bits.ACKEN = 1;
}

// §25.2.3
void HandleI2C()
{
    // Clear the interrupt flag
    PIR1bits.SSP1IF = 0;
    
    if (!SSP1STATbits.D_nA)
    {
        //
        // Handle address message
        //
        
        // The buffer MUST be read to clear SSPxSTAT.BF (§25.2.3.6.1).
        uint8_t devNull = SSP1BUF;
        SendAckI2C();
    }
    else if ((!SSP1STATbits.R_nW) && (SSP1STATbits.BF))
    {
        //
        // Handle data writes
        //

        gDataI2C = SSP1BUF;
        gNewDataI2C = 1;
        
        SendAckI2C();
    }
    
    // Release the clock stretch (§25.2.3.6.1)
    SSP1CON1bits.CKP = 1;
}

void __interrupt() ISR()
{
    if (PIR1bits.SSP1IF == 1) HandleI2C();
}

void UpdateCathodePins(uint8_t bcd)
{
    CATHODE_0_PIN = 0 == bcd;
    CATHODE_1_PIN = 1 == bcd;
    CATHODE_2_PIN = 2 == bcd;
    CATHODE_3_PIN = 3 == bcd;
    CATHODE_4_PIN = 4 == bcd;
    CATHODE_5_PIN = 5 == bcd;
    CATHODE_6_PIN = 6 == bcd;
    CATHODE_7_PIN = 7 == bcd;
    CATHODE_8_PIN = 8 == bcd;
    CATHODE_9_PIN = 9 == bcd;
    
    // TODO: Smooth fading between digits.
}

void main(void)
{
    InitInterrupts();
    InitPins();
    
    // TODO: Read I2C address wired to pins.
    
    InitI2C();
    
    while(1)
    {
        if (gNewDataI2C)
        {
            uint8_t data = gDataI2C;
            gNewDataI2C = 0;
            
            UpdateCathodePins(data);
        }
        
        __delay_ms(10);
    }    
}
