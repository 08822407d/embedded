#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>
#include <OneButton.h>
#include <arduino-timer.h>

	#define PIN_INPUT			12
	#define BTN_LONGPRESS_MS	1000


	OneButton UserBtn(PIN_INPUT, true);
	Timer<1> UserBtnAckLongPressTimer;

	// save the millis when a press has started.
	bool LongPressed = false;
	bool SingleClick = false;
	bool DoubleClick = false;
	bool TripleClick = false;


	void IRAM_ATTR checkTicks() {
		UserBtn.tick();
	}


	bool UserBtn_AckLongPress(void *args) {
		LongPressed = true;
		Serial.println("LongPress Acknowledged.");
	#ifdef Enable_longPress_Continuous
		UserBtn.reset();
	#endif
		return true;
	}

	void singleClick() {
		SingleClick = true;
		Serial.println("singleClick() detected.");
	}

	void doubleClick() {
		DoubleClick = true;
		Serial.println("doubleClick() detected.");
	}

	void tripleClick() {
		TripleClick = true;
		Serial.println("tripleClick detected.");
	}

	void multiClick() {
		int n = UserBtn.getNumberClicks();
		if (n == 3) {
			tripleClick();
		} else if (n == 4) {
			Serial.println("quadrupleClick detected.");
		} else {
			Serial.print("multiClick(");
			Serial.print(n);
			Serial.println(") detected.");
		}
	}

	void pressStart() {
		UserBtnAckLongPressTimer.at(millis() + BTN_LONGPRESS_MS, UserBtn_AckLongPress);
		Serial.println("LongPressStart detected.");
	}
	void pressStop() {
		Serial.println("LongPressEnd detected.");
	}


	void InitUserButton() {
		attachInterrupt(digitalPinToInterrupt(PIN_INPUT), checkTicks, FALLING);
		UserBtn.setClickMs(100);
		UserBtn.attachClick(singleClick);
		// UserBtn.attachDoubleClick(doubleClick);
		// UserBtn.attachMultiClick(multiClick);
		UserBtn.setPressMs(500); // that is the time when LongPressStart is called
		UserBtn.attachLongPressStart(pressStart);
		UserBtn.attachLongPressStop(pressStop);
	}

#endif /* _BUTTON_H_ */