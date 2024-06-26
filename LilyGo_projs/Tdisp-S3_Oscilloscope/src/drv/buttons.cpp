#include "../headers.h"

#include <esp_sleep.h>

#ifdef AMOLED
#  include "pins_config.h"
#endif

#if SCREEN_ROTATION == 1
	OneButton BtnEnter(PIN_BUTTON_2, true);
	OneButton BtnBack(PIN_BUTTON_1, true);
#else
	OneButton BtnEnter(PIN_BUTTON_1, true);
	OneButton BtnBack(PIN_BUTTON_2, true);
#endif

Timer<1> BtnEnter_AckLongPressTimer;
Timer<1> BtnBack_AckLongPressTimer;

unsigned long BtnEnter_PressStartTime = 0;
unsigned long BtnBack_PressStartTime = 0;

ButtonState BtnStat;


/// @brief Button on Pin-14 Functions
/// @param args 
/// @return 
bool BtnEnter_AckLongPress(void *args) {
	BtnStat.btnok = true;
#ifdef Enable_longPress_Continuous
	BtnEnter.reset();
#endif
	return true;
}
void IRAM_ATTR BtnEnter_CheckTicks() {
	BtnEnter.tick(); // just call tick() to check the state.
}
void BtnEnter_SingleClick() {
	BtnStat.btnpl++;
}
void BtnEnter_PressStart() {
	// btnok = true;
	BtnEnter_AckLongPressTimer.at(millis() + BtnLongPressMs, BtnEnter_AckLongPress);
}
void BtnEnter_PressStop() {
}


/// @brief Button on Pin-0 Functions
/// @param args 
/// @return 
bool BtnBack_AckLongPress(void *args) {
	BtnStat.btnbk = true;
#ifdef Enable_longPress_Continuous
	BtnBack.reset();
#endif
	return true;
}
void IRAM_ATTR BtnBack_CheckTicks() {
	BtnBack.tick(); // just call tick() to check the state.
}
void BtnBack_SingleClick() {
	BtnStat.btnmn++;
}
void BtnBack_MultiClick() {
	int n = BtnBack.getNumberClicks();
	Serial.println("MultiClick: " + String(n));
	if (n >= 3)
		esp_deep_sleep_start();
}
void BtnBack_PressStart() {
	// btnbk = true;
	BtnBack_AckLongPressTimer.at(millis() + BtnLongPressMs, BtnBack_AckLongPress);
}
void BtnBack_PressStop() {
}


void InitUserButton() {
	attachInterrupt(digitalPinToInterrupt(BtnEnter.pin()), BtnEnter_CheckTicks, FALLING);
	BtnEnter.setClickMs(50);
	BtnEnter.attachClick(BtnEnter_SingleClick);
	BtnEnter.setPressMs(BtnLongPressMs);
	BtnEnter.attachLongPressStart(BtnEnter_PressStart);
	BtnEnter.attachLongPressStop(BtnEnter_PressStop);

	attachInterrupt(digitalPinToInterrupt(BtnBack.pin()), BtnBack_CheckTicks, FALLING);
	BtnBack.setDebounceMs(100);
	BtnBack.setClickMs(200);
	BtnBack.attachClick(BtnBack_SingleClick);
	BtnBack.attachMultiClick(BtnBack_MultiClick);
	BtnBack.setPressMs(BtnLongPressMs);
	BtnBack.attachLongPressStart(BtnBack_PressStart);
	BtnBack.attachLongPressStop(BtnBack_PressStop);
}