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

#define TOPPS_(RA) RA ## PPS
#define TOPPS(RA) TOPPS_(RA)

#define CATHODE_0_PPS TOPPS(CATHODE_0_PIN)
#define CATHODE_1_PPS TOPPS(CATHODE_1_PIN)
#define CATHODE_2_PPS TOPPS(CATHODE_2_PIN)
#define CATHODE_3_PPS TOPPS(CATHODE_3_PIN)
#define CATHODE_4_PPS TOPPS(CATHODE_4_PIN)
#define CATHODE_5_PPS TOPPS(CATHODE_5_PIN)
#define CATHODE_6_PPS TOPPS(CATHODE_6_PIN)
#define CATHODE_7_PPS TOPPS(CATHODE_7_PIN)
#define CATHODE_8_PPS TOPPS(CATHODE_8_PIN)
#define CATHODE_9_PPS TOPPS(CATHODE_9_PIN)

const uint8_t PWM_MAX = 255;
const uint8_t PWM_RAMP_STEPS = 5;
const uint8_t PWM_RAMP_TIME = 150;
const uint8_t PWM_RAMP_STEP_SIZE = PWM_MAX / PWM_RAMP_STEPS;
const uint8_t PWM_RAMP_STEP_INTERVAL = PWM_RAMP_TIME / PWM_RAMP_STEPS;

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
    SSP1ADD = (uint8_t)(gAddressI2c << 1);
    SSP1MSK = 0xFE;

    // Enable Interrupts
    PIE1bits.SSP1IE = 1;

    // Enable the port (§25.4.5)
    SSP1CON1bits.SSPEN = 1;
}

void InitPWM()
{
    //
    // 1 Hz cycle time
    //
    
    // Use the F_osc/4 source, as required for PWM (§21.10.5, §23.9)
    T2CLKCON = 0x1;
    
    // 250 tick counter reset results (§21.10.2)
    T2PR = 250;
    
    // Mode is free-running, period-pulse, software-gated (§21.10.4)
    T2HLT = 0x00;    
    
    // Enable the timer with a 1:1 pre-scaler (§21.10.3)
    T2CON = 0x80 | 0x00 | 0x00;

    // Enable the PWMs (PWM4 inverted §23.11.1)
    PWM3CON = 0x80;
    PWM4CON = 0x90;
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

void SetPwmDutyCycle(int dc)
{
    // §23.11.2
    PWM3DCH = PWM4DCH = (uint8_t)((dc >> 2) & 0xFF);
    PWM3DCL = PWM4DCL = (uint8_t)((dc & 0x3) << 6);

    // Restart the PWM timer
    T2TMR = 0;
}

#define ASSIGN_PPS(PPS, X) PPS = (X == bcd) ? 3 : (X == lastBcd) ? 4 : 0;

void RampCathodePins(const uint8_t bcd)
{
    static uint8_t lastBcd = 0xFF;
    
    if (bcd == lastBcd) return;

    SetPwmDutyCycle(0);

    ASSIGN_PPS(CATHODE_0_PPS, 0);
    ASSIGN_PPS(CATHODE_1_PPS, 1);
    ASSIGN_PPS(CATHODE_2_PPS, 2);
    ASSIGN_PPS(CATHODE_3_PPS, 3);
    ASSIGN_PPS(CATHODE_4_PPS, 4);
    ASSIGN_PPS(CATHODE_5_PPS, 5);
    ASSIGN_PPS(CATHODE_6_PPS, 6);
    ASSIGN_PPS(CATHODE_7_PPS, 7);
    ASSIGN_PPS(CATHODE_8_PPS, 8);
    ASSIGN_PPS(CATHODE_9_PPS, 9);
    
    // Ramp the duty cycle over time
    for (int pwm = 0; pwm <= PWM_MAX; pwm += PWM_RAMP_STEP_SIZE)
    {
        SetPwmDutyCycle(4 * pwm);
        __delay_ms(PWM_RAMP_STEP_INTERVAL);
    }
    
    lastBcd = bcd;
}

void main(void)
{
    InitInterrupts();
    InitPins();
    InitPWM();

    // TODO: Read I2C address wired to pins.
    
    InitI2C();
    
    while(1)
    {
        if (gNewDataI2C)
        {
            uint8_t data = gDataI2C;
            gNewDataI2C = 0;
            
            RampCathodePins(data);
        }
        
        __delay_ms(10);
    }    
}
