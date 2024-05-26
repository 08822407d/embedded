#include "headers.h"

JoyStick JoyStick1;


void JoyStick::readXY() {
	adc2_get_raw(ADC2_CHANNEL_2, ADC_WIDTH_BIT_12, &JoyStick1.X);
	adc2_get_raw(ADC2_CHANNEL_3, ADC_WIDTH_BIT_12, &JoyStick1.Y);
}