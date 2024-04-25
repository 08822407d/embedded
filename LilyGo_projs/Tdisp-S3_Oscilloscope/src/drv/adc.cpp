#include "headers.h"


static const char *TAG = "ADC DMA";

static uint16_t adc1_chan_mask = BIT(0);
static adc_channel_t channel[1] = { ADC_CHANNEL_0 };

float cal_to_volt_factor = ADC_VOLTREAD_CAP / (1000.0 * ADC_READ_MAX_VAL);

SignalInfo CurrentWave;


void config_adc() {
	esp_wifi_stop();

	esp_err_t ret;

	adc_digi_init_config_t adc_dma_config = {
		.max_store_buf_size = ONE_SAMPLE_BUFFLEN,
		.conv_num_each_intr = ONE_SAMPLE_TIMES,
		.adc1_chan_mask = adc1_chan_mask,
	};
	ESP_ERROR_CHECK(ret = adc_digi_initialize(&adc_dma_config));

	adc_digi_configuration_t dig_cfg = {
		.conv_limit_en = ADC_CONV_LIMIT_EN,
		.conv_limit_num = 250,
		.sample_freq_hz = ADC_SAMPLE_RATE,
		.conv_mode = ADC_CONV_SINGLE_UNIT_1,
		.format = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
	};

	adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
	dig_cfg.pattern_num = sizeof(channel) / sizeof(adc_channel_t);
	for (int i = 0; i < dig_cfg.pattern_num; i++) {
		uint8_t unit = GET_UNIT(channel[i]);
		uint8_t ch = channel[i] & 0x7;
		adc_pattern[i].atten = ADC_ATTEN_DB_11;
		adc_pattern[i].channel = ch;
		adc_pattern[i].unit = unit;
		adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

		// ESP_LOGI(TAG, "adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
		// ESP_LOGI(TAG, "adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
		// ESP_LOGI(TAG, "adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
	}
	dig_cfg.adc_pattern = adc_pattern;
	ESP_ERROR_CHECK(ret = adc_digi_controller_configure(&dig_cfg));
}



float to_voltage(uint32_t reading) {
	return reading * cal_to_volt_factor;
}

// return sample rate in Hz
bool ADC_Sampling(SignalInfo *Wave){
	esp_err_t ret;
	bool sample_valid;

	unsigned long time_start = micros();

	do {
		uint32_t ret_num = 0;

		adc_digi_start();
		ret = adc_digi_read_bytes((uint8_t *)(Wave->SampleBuff),
					ONE_SAMPLE_BUFFLEN, &ret_num, ADC_MAX_DELAY);
		adc_digi_stop();

		Wave->SampleNum = ret_num / ADC_RESULT_BYTE;
		sample_valid = Wave->SampleNum  >= (SAMPLE_VALID_FACTOR * ScreenWidth);
	} while (ret != ESP_OK  || !sample_valid);

	unsigned long sample_timespan = micros() - time_start;

	peak_mean(Wave);
	trigger_freq(Wave);
	genDrawBuffer(Wave);

	unsigned long analyze_timespan = micros() - time_start - sample_timespan;

	Serial.printf("Sample time: %.4f ms; Analyze time: %.4fms\n",
			(sample_timespan / 1000.0), analyze_timespan);

	return sample_valid;
}


