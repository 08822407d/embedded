#include "headers.h"

// #include <driver/adc.h>
#include <esp_adc_cal.h>


#define TIMES				256
#define GET_UNIT(x)			((x>>3) & 0x1)

#define ADC_RESULT_BYTE		4
#define ADC_CONV_LIMIT_EN	0
#define ADC_OUTPUT_TYPE		ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define ADC_CONV_MODE		ADC_CONV_SINGLE_UNIT_1

static uint16_t adc1_chan_mask = BIT(2) | BIT(3);
static uint16_t adc2_chan_mask = 0;
static adc_channel_t channel[2] = {ADC_CHANNEL_2, ADC_CHANNEL_3};

static const char *TAG = "ADC DMA";

esp_adc_cal_characteristics_t	adc_chars;

void characterize_adc() {
	esp_adc_cal_characterize(
		ADC_UNIT_1,
		ADC_ATTEN_DB_11,
		ADC_WIDTH_BIT_12,
		1100,
		&adc_chars
	);

	// adc_digi_init_config_t adc_dma_config = {
	// 	.max_store_buf_size = 1024,
	// 	.conv_num_each_intr = TIMES,
	// 	.adc1_chan_mask = adc1_chan_mask,
	// 	.adc2_chan_mask = adc2_chan_mask,
	// };
	// ESP_ERROR_CHECK(adc_digi_initialize(&adc_dma_config));

	// adc_digi_configuration_t dig_cfg = {
	// 	.conv_limit_en = ADC_CONV_LIMIT_EN,
	// 	.conv_limit_num = 250,
	// 	.sample_freq_hz = 20 * 1000,
	// 	.conv_mode = ADC_CONV_MODE,
	// 	.format = ADC_OUTPUT_TYPE,
	// };

	// adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
	// dig_cfg.pattern_num = sizeof(channel) / sizeof(adc_channel_t);
	// for (int i = 0; i < dig_cfg.pattern_num; i++) {
	// 	uint8_t unit = GET_UNIT(channel[i]);
	// 	uint8_t ch = channel[i] & 0x7;
	// 	adc_pattern[i].atten = ADC_ATTEN_DB_0;
	// 	adc_pattern[i].channel = ch;
	// 	adc_pattern[i].unit = unit;
	// 	adc_pattern[i].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;

	// 	ESP_LOGI(TAG, "adc_pattern[%d].atten is :%x", i, adc_pattern[i].atten);
	// 	ESP_LOGI(TAG, "adc_pattern[%d].channel is :%x", i, adc_pattern[i].channel);
	// 	ESP_LOGI(TAG, "adc_pattern[%d].unit is :%x", i, adc_pattern[i].unit);
	// }
	// dig_cfg.adc_pattern = adc_pattern;
	// ESP_ERROR_CHECK(adc_digi_controller_configure(&dig_cfg));
}


// return sample rate in Hz
unsigned long ADC_Sampling(uint16_t *adc_buffer){
	unsigned long timespan;
	unsigned long sample_freq;
	unsigned long time_start = micros();

	for (int i = 0; i < BUFF_SIZE; i++) {
		uint32_t raw = analogRead(SIGNAL_INPUT_PIN);
		adc_buffer[i] = (uint16_t)esp_adc_cal_raw_to_voltage(raw, &adc_chars);
	}

	// esp_err_t ret;
	// uint32_t ret_num = 0;
	// uint8_t result[TIMES] = {0};
	// memset(result, 0xcc, TIMES);

	// adc_digi_start();

	// ret = adc_digi_read_bytes(result, TIMES, &ret_num, ADC_MAX_DELAY);

	// adc_digi_stop();
	// ret = adc_digi_deinitialize();

	timespan = micros() - time_start;

	// Serial.printf("Sample time: %lu ms\n", timespan / 1000);
	// String vals;
	// for (int i = 0; i < BUFF_SIZE; i+=(BUFF_SIZE / 10))
	// 	vals += String(adc_buffer[i]) + " ";
	// Serial.println("Vals: %s\n" + vals);

	if (timespan == 0)
		return 0;
	else
		return (1000000UL * BUFF_SIZE) / timespan;
}


