#include "headers.h"

// #include <driver/adc.h>
#include <esp_adc_cal.h>


esp_adc_cal_characteristics_t	adc_chars;

void characterize_adc() {
	esp_adc_cal_characterize(
		ADC_UNIT_1,
		ADC_ATTEN_DB_11,
		ADC_WIDTH_BIT_12,
		1100,
		&adc_chars
	);
}


unsigned long ADC_Sampling(uint16_t *adc_buffer){
	unsigned long timespan;
	unsigned long sample_freq;
	unsigned long time_start = micros();
	for (int i = 0; i < BUFF_SIZE; i++) {
		uint32_t raw = analogRead(PIN_BAT_VOLT);
		adc_buffer[i] = (uint16_t)esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2;
	}
	timespan = micros() - time_start;
	if (timespan == 0)
		return 0;
	else
		return (1000000UL * BUFF_SIZE) / timespan;
}