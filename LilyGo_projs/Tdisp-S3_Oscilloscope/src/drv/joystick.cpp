#include "headers.h"

JoyStick JoyStick1;


void JoyStick::readXY() {
	adc2_get_raw(ADC2_CHANNEL_5, ADC_WIDTH_BIT_12, &JoyStick1.X);
	adc2_get_raw(ADC2_CHANNEL_6, ADC_WIDTH_BIT_12, &JoyStick1.Y);
}