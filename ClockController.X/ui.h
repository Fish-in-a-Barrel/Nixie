#ifndef UI_H
#define	UI_H

///
/// Causes the spinner on the display to rotate one step as proof of life.
void UI_TickSpinner(void);

///
/// Updates the UI in response to the encoder being rotated clockwise.
void UI_HandleRotationCW(void);

///
/// Updates the UI in response to the encoder being rotated counter-clockwise.
void UI_HandleRotationCCW(void);

///
/// Updates the UI in response to the encoder being depressed.
void UI_HandleButtonPress(void);

///
/// Triggers a redraw of the UI with fresh data from sources.
void UI_Update(void);

#endif	/* UI_H */

