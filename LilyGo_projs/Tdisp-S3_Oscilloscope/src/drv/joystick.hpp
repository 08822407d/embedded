#pragma once

#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_wifi.h>

class JoyStick {
public:
	int    X;
	int    Y;


	void readXY();
};


extern JoyStick JoyStick1;