/**
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5StickCPlus2: https://github.com/m5stack/M5StickCPlus2
 */

#include <Arduino.h>
#include <M5StickCPlus2.h>
#include <UNIT_MiniJoyC.hpp>
#include <M5Unit_ScrollEncoder.hpp>


UNIT_JOYC joyc;
M5UnitScroll scroll;


void setup()
{   
	auto cfg = M5.config();
	StickCP2.begin(cfg);
	Serial.begin(115200);
	Serial.println("M5StickCPlus2 initialized");
	delay(2000);

	Wire.end();
	Wire1.end();

	while (!(joyc.begin(&Wire, JoyC_ADDR, 0, 26, 100000UL))) {
		delay(100);
		Serial.println("I2C Error!\r\n");
	}
	while (!(scroll.begin(&Wire1, UNIT_SCROLL_ADDR, 32, 33, 400000UL))) {
		delay(100);
		Serial.println("I2C Error!\r\n");
	}
}

void loop()
{
	uint16_t adc_x = joyc.getADCValue(POS_X);
	uint16_t adc_y = joyc.getADCValue(POS_Y);

	int16_t encoder_value = scroll.getEncoderValue();
	bool btn_stauts       = scroll.getButtonStatus();

	Serial.printf("ADC X: %d, ADC Y: %d, Encoder Value: %d\n", adc_x, adc_y, encoder_value);

	delay(20);
}