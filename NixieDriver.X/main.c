//
// Section references are to the "PIC16F15213/14/23/24/43/44 Low Pin Count Microcontrollers" datasheet.
// Document DS40002195D
// https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/PIC16F15213-14-23-24-43-44-Microcontroller-Data-Sheet-40002195.pdf
//

#include <xc.h>
#include "clock.h"
#include "pps_inputs.h"
#include "pps_outputs.h"

// The pin GPIO register for each cathode driver.
#define CATHODE_0_PIN RC3
#define CATHODE_9_PIN RC6
#define CATHODE_8_PIN RC7
#define CATHODE_7_PIN RB7
#define CATHODE_6_PIN RC4
#define CATHODE_5_PIN RC5
#define CATHODE_4_PIN RA4
#define CATHODE_3_PIN RA5
#define CATHODE_2_PIN RA2
#define CATHODE_1_PIN RC2
#define CATHODE_COMMA_PIN RB5

// The pin GPIO register for each address bit.
#define ADDRESS_PIN_0 RA0 // LSB
#define ADDRESS_PIN_1 RA1
#define ADDRESS_PIN_2 RC1
#define ADDRESS_PIN_3 RC0

#define I2C_ADDRESS (\
    (uint8_t)((uint8_t)ADDRESS_PIN_0 << 1) | \
    (uint8_t)((uint8_t)ADDRESS_PIN_1 << 2) | \
    (uint8_t)((uint8_t)ADDRESS_PIN_2 << 3) | \
    (uint8_t)((uint8_t)ADDRESS_PIN_3 << 4) )

// TOPPS(X) composites the pin register with the suffix "PPS" to form the PPS register associated with that pin.
#define TOPPS_(RA) RA ## PPS
#define TOPPS(RA) TOPPS_(RA)

// The PPS register for each driver pin.
#define CATHODE_1_PPS TOPPS(CATHODE_1_PIN)
#define CATHODE_2_PPS TOPPS(CATHODE_2_PIN)
#define CATHODE_3_PPS TOPPS(CATHODE_3_PIN)
#define CATHODE_4_PPS TOPPS(CATHODE_4_PIN)
#define CATHODE_5_PPS TOPPS(CATHODE_5_PIN)
#define CATHODE_6_PPS TOPPS(CATHODE_6_PIN)
#define CATHODE_7_PPS TOPPS(CATHODE_7_PIN)
#define CATHODE_8_PPS TOPPS(CATHODE_8_PIN)
#define CATHODE_9_PPS TOPPS(CATHODE_9_PIN)
#define CATHODE_0_PPS TOPPS(CATHODE_0_PIN)
#define CATHODE_COMMA_PPS TOPPS(CATHODE_COMMA_PIN)

const int PWM_MAX = 250; 
const int PWM_RAMP_STEPS = 5;
const int PWM_RAMP_TIME = 150;
const int PWM_RAMP_STEP_SIZE = PWM_MAX / PWM_RAMP_STEPS;
const int PWM_RAMP_STEP_INTERVAL = PWM_RAMP_TIME / PWM_RAMP_STEPS;

void InitPins()
{
    /*
     * I2C:
     * SCL = RB4
     * SDA = RB6
     * 
     * Address:
     * A0 = RA0
     * A1 = RA1
     * A2 = RC1
     * A3 = RC3
     */
    
    // I2C pins must be configured as inputs (�25.2.2.3)
    // Address pins are inputs
    TRISA = 0x03;
    TRISB = 0x50;
    TRISC = 0x03;

    // Clear the analog registers (�16.5)
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    
    // Clear the GPIO pins
    PORTA = 0;
    PORTB = 0;
    PORTC = 0;
    
    // Set PPS input pins (�18.2, Table 18-1; �18.8.1)
    SSP1CLKPPS = PPS_INPUT(PPS_PORT_B, 4);
    SSP1DATPPS = PPS_INPUT(PPS_PORT_B, 6);

    // Set PPS output pins (�18.8.2)
    RB4PPS = PPS_OUT_SCL1;
    RB6PPS = PPS_OUT_SDA1;
}

void InitInterrupts()
{
    // Enable global and peripheral interrupts (�12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 
}

void InitI2C()
{
    // Not relevant to clients (�25.4.4)
    SSP1STAT = 0x00;
    
    // 7-bit addressing client (�25.4.5)
    SSP1CON1 = 0x06;
    
    // Enable clock stretching (�25.4.6)
    SSP1CON2 = 0x01;
    
    // No start/stop interrupts (�25.4.7)
    SSP1CON3 = 0x00;
    
    // Set the client address and enable the full address mask (�25.4.2, �25.4.3)
    SSP1ADD = I2C_ADDRESS;
    SSP1MSK = 0xFE;

    // Enable Interrupts
    PIE1bits.SSP1IE = 1;

    // Enable the port (�25.4.5)
    SSP1CON1bits.SSPEN = 1;
}

void InitPWM()
{
    // Use the F_osc/4 source, as required for PWM (�21.10.5, �23.9)
    T2CLKCON = 0x1;
    
    // 250 tick counter reset results (�21.10.2)
    T2PR = (uint8_t)PWM_MAX;
    
    // Mode is free-running, period-pulse, software-gated (�21.10.4)
    T2HLT = 0x00;    
    
    // Enable the timer with a 1:16 pre-scaler (�21.10.3)
    T2CONbits.CKPS = 0;
    T2CONbits.ON = 1;

    // Invert PWM 4 (�23.11.1)
    PWM4CONbits.POL = 1;
    
    // Enable the PWMs (�23.11.1)
    PWM3CONbits.EN = 1;
    PWM4CONbits.EN = 1;
}

volatile uint8_t gDataI2C = 0;
volatile uint8_t gNewDataI2C = 0;

// �25.2.3
void HandleI2C()
{
    // Clear the interrupt flag
    PIR1bits.SSP1IF = 0;
    
    if (!SSP1STATbits.D_nA)
    {
        //
        // Handle address message
        //
        
        // The buffer MUST be read to clear SSPxSTAT.BF (�25.2.3.6.1).
        uint8_t devNull = SSP1BUF;
        
        // If this is a read, immediately respond with the first byte (�25.2.3.7.2).
        if (SSP1STATbits.R_nW) SSP1BUF = gDataI2C;

        // Release the clock stretch (�25.2.3.6.1)
        SSP1CON1bits.CKP = 1;
    }
    else if (SSP1STATbits.BF)
    {
        //
        // Handle data - write
        //

        uint8_t buf = SSP1BUF;
        if (buf != gDataI2C)
        {
            gDataI2C = buf;
            gNewDataI2C = 1;
        }

        // Release the clock stretch. (�25.2.3.6.1)
        // The data sheet does not say to do this here, but the bus locks up 100% of the time if I don't.
        SSP1CON1bits.CKP = 1;
    }
}

void __interrupt() ISR()
{
    if (PIR1bits.SSP1IF == 1) HandleI2C();
}

void SetPwmDutyCycle(int dc)
{
    // �23.11.2
    PWM3DCH = PWM4DCH = (uint8_t)dc;
    PWM3DCL = PWM4DCL = 0;

    // Restart the PWM timer
    T2TMR = 0;
}

// Expands to an assignment that sets the PPS output for a given pin to one of three values:
//   If the pin # matches the selected digit, assign PWM 3 (increasing duty cycle, fade in)
//   Otherwise, if the pin # matches the previously selected digit, assign PWM 4 (decreasing duty cycle, fade out)
//   Otherwise, assign GPIO (off)
#define ASSIGN_PPS(PPS, X) PPS = (X == bcd) ? 3 : (X == lastBcd) ? 4 : 0;

void RampCathodePins(uint8_t bcd)
{
    static uint8_t lastBcd = 0xFF;
    
    // Set the comma driver
    CATHODE_COMMA_PIN = bcd >> 7;
    bcd &= 0x7F;
    
    if (bcd == lastBcd) return;

    SetPwmDutyCycle(0);

    // Set the digit drivers.
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
        SetPwmDutyCycle(pwm);
        __delay_ms(PWM_RAMP_STEP_INTERVAL);
    }
    
    // Set the duty cycle > max for 100% duty cycle
    SetPwmDutyCycle(PWM_MAX + 1);
    
    lastBcd = bcd;
}

// Scroll through all the cathodes for a minute. Apparently cathodes that aren't used can fail.
void RefreshCathodes()
{
    CATHODE_0_PIN = 0;
    CATHODE_1_PIN = 0;
    CATHODE_2_PIN = 0;
    CATHODE_3_PIN = 0;
    CATHODE_4_PIN = 0;
    CATHODE_5_PIN = 0;
    CATHODE_6_PIN = 0;
    CATHODE_7_PIN = 0;
    CATHODE_8_PIN = 0;
    
    for (uint8_t i = 0; i < 6; ++i)
    {
        CATHODE_9_PIN = 0;
        CATHODE_0_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_0_PIN = 0;
        CATHODE_1_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_1_PIN = 0;
        CATHODE_2_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_2_PIN = 0;
        CATHODE_3_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_3_PIN = 0;
        CATHODE_4_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_4_PIN = 0;
        CATHODE_5_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_5_PIN = 0;
        CATHODE_6_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_6_PIN = 0;
        CATHODE_7_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_7_PIN = 0;
        CATHODE_8_PIN = 1;
        __delay_ms(1000);
        
        CATHODE_8_PIN = 0;
        CATHODE_9_PIN = 1;
        __delay_ms(1000);
    }

    CATHODE_9_PIN = 0;
}

void main(void)
{
    InitClock();
    InitInterrupts();
    InitPins();
    InitPWM();
    
    // Give the address pins time to stabilize.
    __delay_ms(50);
    
    InitI2C();

    while(1)
    {
        if (gNewDataI2C)
        {
            uint8_t data = gDataI2C;
            gNewDataI2C = 0;
            
            if (0xFF != data) RampCathodePins(data);
            else RefreshCathodes();
        }
        
        __delay_ms(10);
    }
}
