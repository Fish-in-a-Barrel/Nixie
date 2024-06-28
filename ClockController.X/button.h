#ifndef BUTTON_H
#define	BUTTON_H

#include <xc.h>

union ButtonState
{
    struct
    {
        uint8_t state : 1;      // The current debounced state of the button
        uint8_t edge : 1;       // 1 if the state changed on the last update
    };

    uint8_t _raw;
};

#define ROTATION_NONE 0
#define ROTATION_CW 1
#define ROTATION_CCW 2

struct
{
    union ButtonState a;
    union ButtonState b;
    union ButtonState c;
    
    uint8_t rotation;
    volatile int8_t deltaR; // CCW < 0 < CW
} gButtonState;

void Buttons_Init(void);

void Buttons_HandleInterrupt(void);

#endif	/* BUTTON_H */

