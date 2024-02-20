#ifndef BUTTON_H
#define	BUTTON_H

#define BUTTON_STATE_HELD 0
#define BUTTON_STATE_RELEASED 1

extern uint8_t gButtonState;
extern uint8_t gLongPress;

void InitButton(void);

void UpdateButtonState(void);

#endif	/* BUTTON_H */