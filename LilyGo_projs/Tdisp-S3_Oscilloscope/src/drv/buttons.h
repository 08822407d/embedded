#pragma once

#include <OneButton.h>
#include <arduino-timer.h>


#define PIN_BUTTON_1		0
#define PIN_BUTTON_2		14
#define PIN_BAT_VOLT		4
#define BtnLongPressMs      500

class ButtonState {
public:
	bool	btnok = false;
	bool	btnbk = false;
	uint	btnpl = 0;
	uint	btnmn = 0;
};

/* buttons.cpp */
extern ButtonState BtnStat;
extern OneButton BtnBack;
extern OneButton BtnEnter;

extern Timer<1> BtnEnter_AckLongPressTimer;
extern Timer<1> BtnBack_AckLongPressTimer;