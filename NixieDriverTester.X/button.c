#include <xc.h>

#include "button.h"

#define TRANSITION_TIME 5

static uint8_t gButtonState = 0xff;
static uint8_t gTransitionCounter = 0;

uint8_t GetButtonState(void)
{
#ifndef BREADBOARD
    uint8_t state = RA2;
#else
    uint8_t state = RB6;
#endif
    
    if (gButtonState == 0xff) gButtonState = !state;
    
    if (state != (gButtonState & 0x01))
    {
        if (++gTransitionCounter > TRANSITION_TIME)
        {
            gButtonState = state;
            state |= BUTTON_STATE_CHANGED;
            gTransitionCounter = 0;
        }
    }
    
    return state;
}
