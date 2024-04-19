#include "../functions.h"

#include <esp_sleep.h>


OneButton BtnEnter(PIN_BUTTON_2, true);
OneButton BtnBack(PIN_BUTTON_1, true);

Timer<1> BtnEnter_AckLongPressTimer;
Timer<1> BtnBack_AckLongPressTimer;

unsigned long BtnEnter_PressStartTime = 0;
unsigned long BtnBack_PressStartTime = 0;

int btnok,
	btnpl,
	btnmn,
	btnbk;


/// @brief Button on Pin-14 Functions
/// @param args 
/// @return 
bool BtnEnter_AckLongPress(void *args) {
	btnok = 1;
#ifdef Enable_longPress_Continuous
	BtnEnter.reset();
#endif
	return true;
}
void IRAM_ATTR BtnEnter_CheckTicks() {
	BtnEnter.tick(); // just call tick() to check the state.
}
void BtnEnter_SingleClick() {
	btnpl = 1;
}
void BtnEnter_PressStart() {
	BtnEnter_AckLongPressTimer.at(millis() + BtnLongPressMs, BtnEnter_AckLongPress);
}
void BtnEnter_PressStop() {
}


/// @brief Button on Pin-0 Functions
/// @param args 
/// @return 
bool BtnBack_AckLongPress(void *args) {
	btnbk = 1;
#ifdef Enable_longPress_Continuous
	BtnBack.reset();
#endif
	return true;
}
void IRAM_ATTR BtnBack_CheckTicks() {
	BtnBack.tick(); // just call tick() to check the state.
}
void BtnBack_SingleClick() {
	btnmn = 1;
}
void BtnBack_MultiClick() {
	int n = BtnBack.getNumberClicks();
	if (n >= 4)
		esp_deep_sleep_start();
}
void BtnBack_PressStart() {
	BtnBack_AckLongPressTimer.at(millis() + BtnLongPressMs, BtnBack_AckLongPress);
}
void BtnBack_PressStop() {
}


void InitUserButton() {
	attachInterrupt(digitalPinToInterrupt(BtnEnter.pin()), BtnEnter_CheckTicks, FALLING);
	BtnEnter.setClickMs(100);
	BtnEnter.attachClick(BtnEnter_SingleClick);
	BtnEnter.setPressMs(BtnLongPressMs);
	BtnEnter.attachLongPressStart(BtnEnter_PressStart);
	BtnEnter.attachLongPressStop(BtnEnter_PressStop);

	attachInterrupt(digitalPinToInterrupt(BtnBack.pin()), BtnBack_CheckTicks, FALLING);
	BtnBack.setClickMs(250);
	BtnBack.attachClick(BtnBack_SingleClick);
	BtnBack.attachMultiClick(BtnBack_MultiClick);
	BtnBack.setPressMs(BtnLongPressMs);
	BtnBack.attachLongPressStart(BtnBack_PressStart);
	BtnBack.attachLongPressStop(BtnBack_PressStop);
}