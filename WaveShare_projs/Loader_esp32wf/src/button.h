#ifndef _BUTTON_H_
#define _BUTTON_H_

#include <Arduino.h>
#include <OneButton.h>
#include "esp32-waveshare-epd/utility/EPD_2in13_V3.h"

	#define PIN_INPUT 12


	OneButton button(PIN_INPUT, true);

	// save the millis when a press has started.
	unsigned long pressStartTime;
	bool hasSingleClick = false;
	bool hasDoubleClick = false;
	bool hasTripleClick = false;


	void IRAM_ATTR checkTicks() {
		// include all buttons here to be checked
		button.tick(); // just call tick() to check the state.
	}


	// this function will be called when the button was pressed 1 time only.
	void singleClick() {
		hasSingleClick = true;
		Serial.println("singleClick() detected.");
	} // singleClick

	// this function will be called when the button was pressed 2 times in a short timeframe.
	void doubleClick() {
		hasDoubleClick = true;
		Serial.println("doubleClick() detected.");

	} // doubleClick

	void tripleClick() {
		hasTripleClick = true;
		Serial.println("tripleClick detected.");
	}

	// this function will be called when the button was pressed multiple times in a short timeframe.
	void multiClick() {
		int n = button.getNumberClicks();
		if (n == 3) {
			tripleClick();
		} else if (n == 4) {
			Serial.println("quadrupleClick detected.");
		} else {
			Serial.print("multiClick(");
			Serial.print(n);
			Serial.println(") detected.");
		}

	} // multiClick

	// this function will be called when the button was held down for 1 second or more.
	void pressStart() {
		Serial.println("pressStart()");
		pressStartTime = millis() - 1000; // as set in setPressMs()
	} // pressStart()

	// this function will be called when the button was released after a long hold.
	void pressStop() {
		Serial.print("pressStop(");
		Serial.print(millis() - pressStartTime);
		Serial.println(") detected.");
	} // pressStop()


	void InitUserButton() {
		attachInterrupt(digitalPinToInterrupt(PIN_INPUT), checkTicks, FALLING);
		button.setClickMs(300);
		button.setPressMs(500);
		// link the xxxclick functions to be called on xxxclick event.
		button.attachClick(singleClick);
		button.attachDoubleClick(doubleClick);
		button.attachMultiClick(multiClick);
		button.setPressMs(1000); // that is the time when LongPressStart is called
		button.attachLongPressStart(pressStart);
		button.attachLongPressStop(pressStop);
	}

#endif /* _BUTTON_H_ */