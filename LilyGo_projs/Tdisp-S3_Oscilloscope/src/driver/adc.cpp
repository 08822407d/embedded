#include "headers.h"


esp_adc_cal_characteristics_t	adc_chars;

void characterize_adc() {
	esp_adc_cal_characterize(
		ADC_UNIT_1,
		// (adc_atten_t)ADC_CHANNEL,
		ADC_ATTEN_DB_11,
		ADC_WIDTH_BIT_12,
		1100,
		&adc_chars
	);
}


void ADC_Sampling(uint32_t *adc_buffer){
	for (int i = 0; i < BUFF_SIZE; i++) {
		uint32_t raw = analogRead(PIN_BAT_VOLT);
		adc_buffer[i] = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2;
	}
}