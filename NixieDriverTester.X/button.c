#include <xc.h>

#include "button.h"
#include "timer.h"

#ifndef BREADBOARD
    #define BUTTON_PIN RA1;
#else
    #define BUTTON_PIN RB6;
#endif

#define TRANSITION_TIME 5

uint8_t gButtonState = BUTTON_STATE_RELEASED;
uint8_t gLongPress = 0;

void InitButtonPins(void)
{
    // Set RA1 as a discrete input for the button, use the internal weak pull-up
    TRISA |= 0x02;
    WPUA |= 0x02;
}

void InitButton(void)
{
    InitButtonPins();
}

void UpdateButtonState(void)
{
    static uint8_t gTransitionCounter = 0;
    static uint32_t holdStartTick = 0;
    
    uint8_t state = BUTTON_PIN;
    
    if (gButtonState == 0xff) gButtonState = state;
    
    if ((state != gButtonState) && (++gTransitionCounter > TRANSITION_TIME))
    {
        gButtonState = state;
        gTransitionCounter = 0;
        
        if (BUTTON_STATE_HELD == state) holdStartTick = gTickCount;
        else gLongPress = gTickCount - holdStartTick > TICK_FREQ;
    }
}
