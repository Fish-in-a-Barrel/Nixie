//
// Section references are to the "PIC16F15213/14/23/24/43/44 Low Pin Count Microcontrollers" datasheet.
// Document DS40002195D
// https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ProductDocuments/DataSheets/PIC16F15213-14-23-24-43-44-Microcontroller-Data-Sheet-40002195.pdf
//

#include <xc.h>

#include "config_bits.h"
#include "clock.h"
#include "pins.h"
#include "button.h"
#include "pwm.h"
#include "i2c.h"
#include "oled.h"
#include "font8x5.h"
#include "adc.h"
#include "timer.h"

// The ADC returns a 10-bit value dividing the range 0mv to 4096mV evenly so V/4 = ADC.
// The expected voltage from the voltage divider is 2,800mV, which means the expected ADC value is 700. This will vary with the exact resistance of
// the resistors in the voltage divider.
#define ADC_SP 715L
#define ADC_DEADBAND 5

uint32_t gAdcAccumulator = 0;
uint16_t gAdcAccumulatorCount = 0;
uint16_t gAdcCv = 0;

// The Nixie tube will be blanked if the high-voltage supply is outside the target range.
#define HV_TARGET 180
#define HV_DEADBAND 10
#define HV_MIN (HV_TARGET - HV_DEADBAND)
#define HV_MAX (HV_TARGET + HV_DEADBAND)

uint8_t gVoltage = 0;

// The voltage booster is controlled by changing the duty-cycle of the PWM signal driving it.
// By scaling the working DC value, we can have a working value with more granularity than the actual PWM allows. This can allow for some smoother
// control, depending on the algorithm.
#define PWM_DC_SCALAR 32
#define PWM_DC_MIN (uint16_t)(0.85 * (TMR2_RESET << 2) * PWM_DC_SCALAR)
#define PWM_DC_MAX (uint16_t)(0.95 * (TMR2_RESET << 2) * PWM_DC_SCALAR)

uint16_t gPwmDutyCycle = PWM_DC_MIN;

typedef struct
{
    uint8_t digit:4; // LSB. The digit to display. Blanks the nixie for values > 9.
    uint8_t _pad:3;  // unused
    uint8_t comma:1; // 1 = comma lit (for IN-12B)
} NixieState;

#define NIXIE_DIGIT_BLANK 0xF

uint8_t gNixieAutoIncrement = 1;
NixieState gCurrentNixieState = { NIXIE_DIGIT_BLANK, 0, 0 };

uint8_t gLastButtonState = BUTTON_STATE_RELEASED;

void __interrupt() ISR()
{
    // Dispatch interrupts to handlers (§12.9.6)
    if (PIR1bits.SSP1IF || PIR1bits.BCL1IF) I2C_HandleInterrupt();
    
    if (PIR1bits.TMR2IF) TimerInterruptHandler();
    
    if (PIR1bits.ADIF)
    {
        PIR1bits.ADIF = 0;
        gAdcAccumulator += ADRES;
        ++gAdcAccumulatorCount;
    }
}

void EnableInterrupts()
{
    // Enable global and peripheral interrupts (§12.4)
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; 

    // Enable MSSP interrupts (§12.9.3)
    PIE1bits.SSP1IE = 1;
    PIE1bits.BCL1IE = 1;
    
    // Enable the TMR2 interrupt for tick counting
    PIE1bits.TMR2IE = 1;

    // Enable ACD interrupt
    PIR1bits.ADIF = 0;
    PIE1bits.ADIE = 1;
}

void CaptureAdc(void)
{
    if (0 == gAdcAccumulatorCount)
    {
        gVoltage = 0;
        return;
    }
    
    uint32_t accum;
    uint32_t count;
    
    // Pause global interrupts while reading the ADC value because this is not atomic.
    INTCONbits.GIE = 0;
    accum = gAdcAccumulator;
    count = gAdcAccumulatorCount;
    gAdcAccumulator = 0;
    gAdcAccumulatorCount = 0;
    INTCONbits.GIE = 1;
    
    gAdcCv =(uint16_t)(accum / count);
    
    // (4096 * ADC_raw) / 1024 = mV on pin.
    // multiply by 63.5 to compensate for the voltage divider supplying the pin.
    // divide by 1000 to convert from mV to volts
    // This works out to (63.5 * (4096 / 1024)) / 1000 = 0.254, or ~(1/4 + 1/250).
    // The exact values used will depend on the precise value of the resistors.
    gVoltage = (uint8_t)(gAdcCv / 4) + (uint8_t)(gAdcCv / 210);
}

void AdjustVoltagePwm(void)
{
    /*
    const int16_t DELTA_PWM_MAX = PWM_SCALAR * 500;
    const int16_t I_MAX = 5 * PWM_SCALAR / 2;
    const int16_t Kp_N = 4;
    const int16_t Kp_D = 5;
    const int16_t Ki_N = 5;
    const int16_t Ki_D = PWM_SCALAR * 4;
    
    static int16_t i = 0;

    int16_t error = PWM_SCALAR * (int16_t)(ADC_SP - gAdcCv);
    int16_t p = (error * Kp_N) / Kp_D;
    i += error;
    
    // prevent integral wind-up
    //i = (9 * i) / 10;
    if (i > I_MAX) i = I_MAX;
    if (i < -I_MAX) i = -I_MAX;    
    
    int16_t delta = p + ((Ki_N * i) / Ki_D);
     
    if (delta > DELTA_PWM_MAX) delta = DELTA_PWM_MAX;
    if (delta < -DELTA_PWM_MAX) delta = -DELTA_PWM_MAX;
    
    gPwmDutyCycle = (uint16_t)((int16_t)gPwmDutyCycle + delta);
    */
    
    if (gAdcCv + ADC_DEADBAND < ADC_SP) ++gPwmDutyCycle;
    else if (gAdcCv- ADC_DEADBAND > ADC_SP) --gPwmDutyCycle;
    
    if (gPwmDutyCycle > PWM_DC_MAX) gPwmDutyCycle = PWM_DC_MAX;
    else if (gPwmDutyCycle < PWM_DC_MIN) gPwmDutyCycle = PWM_DC_MIN;
    
    SetPwmDutyCycle(gPwmDutyCycle / PWM_DC_SCALAR);
}

void DisplayNumber(uint16_t number, int8_t digitCount, uint8_t row, uint8_t x)
{
    while (digitCount > 0)
    {
        DrawCharacter(row, (uint8_t)(x + --digitCount), number % 10);
        number /= 10;
    }
}

void UpdateNixieState(void)
{
    static uint32_t nixieStartTickCount = 0;
    static uint32_t commaToggleTickCount = (uint32_t)(~0);
    static uint32_t buttonStartTickCount = 0;

    NixieState targetState = gCurrentNixieState;
    
    if (NIXIE_DIGIT_BLANK == gCurrentNixieState.digit)
    {
        nixieStartTickCount = gTickCount;
        targetState.digit = 0;
    }

    if (gNixieAutoIncrement)
    {
        // This doesn't handle roll-over of the tick counter, but that's not important for this application.
        if (gTickCount - nixieStartTickCount > TICK_FREQ)
        {
            nixieStartTickCount = gTickCount;
            commaToggleTickCount = nixieStartTickCount + TICK_FREQ / 2;
            targetState.digit = (gCurrentNixieState.digit + 1) % 10;
        }

        // Flip the comma state when the toggle tick count is reached.
        if (gTickCount >= commaToggleTickCount)
        {
            targetState.comma = gCurrentNixieState.comma ? 0 : 1;
            commaToggleTickCount = (uint32_t)(~0);
        }
    }

    if ((gLastButtonState != gButtonState) && (BUTTON_STATE_RELEASED == gButtonState))
    {
        if (gLongPress) gNixieAutoIncrement = !gNixieAutoIncrement;
        if (!gNixieAutoIncrement) targetState.digit = (gCurrentNixieState.digit + 1) % 10;
    }
    
    if ((targetState.digit != gCurrentNixieState.digit) || (targetState.comma != gCurrentNixieState.comma))
    {
        I2C_Write(0x0F, &targetState, sizeof(targetState));        

        gCurrentNixieState = targetState;
    }
}

void DrawStaticDisplaySymbols(void)
{
        // Voltage: --- V
    DrawCharacter(0, 14, CHAR_V);
    
    // Duty Cycle: -- %
    DrawCharacter(3, 13, CHAR_SLH);
    DisplayNumber(TMR2_RESET << 2, 4, 3, 15);
}

void RefreshDisplay()
{
    uint32_t lastUpdateTick = 0;
    if (gTickCount - lastUpdateTick < TMR2_FREQ / 10) return;
    lastUpdateTick = gTickCount;
    
    // Scroll * vertically for proof of life
    for (uint8_t i = 0; i < 4; ++i)
        DrawCharacter(i, 20, i == (gTickCount / (TMR2_FREQ / 40)) % 4 ? CHAR_AST : CHAR_SPC);
    
    //
    // Nixie state
    //
    
    uint8_t nixieState = (gCurrentNixieState.digit != NIXIE_DIGIT_BLANK) ? gCurrentNixieState.digit : CHAR_DSH;
    uint8_t autoState = gNixieAutoIncrement ? CHAR_AST : CHAR_SPC;
    
    DrawCharacter(0, 0, nixieState);
    DrawCharacter(0, 2, autoState);

    //
    // Voltage and duty cycle
    //
    
    
    DisplayNumber(gAdcCv, 4, 3, 0);
    DisplayNumber(gVoltage, 3, 0, 10);
    DisplayNumber(gPwmDutyCycle / PWM_DC_SCALAR, 4, 3, 8);

    //
    // Button pressed indicator
    //

    if (gLastButtonState != gButtonState) InvertDisplay(!gButtonState);
}

void main(void)
{
    InitClock();
    InitPins();
    
    // Give other devices time to finish power-up.
    __delay_ms(50);
    
    I2C_Host_Init();
    EnableInterrupts();
    
    SetupDisplay();
    DrawStaticDisplaySymbols();
    
    InitButton();
    InitTimer();
    InitPWM(gPwmDutyCycle / PWM_DC_SCALAR);
    InitAdc();
    
    while (1)
    {
        CaptureAdc();
        AdjustVoltagePwm();
        
        UpdateButtonState();
        UpdateNixieState();
        RefreshDisplay();
        
        gLastButtonState = gButtonState;
        
        __delay_ms(5);
    }
}
