

/* data_analysis.cpp */
extern void peak_mean(uint32_t *adc_buffer, uint32_t len, float * max_value, float * min_value, float *pt_mean);
extern bool digital_analog(uint32_t *adc_buffer, uint32_t max_v, uint32_t min_v);
extern void trigger_freq_analog(uint32_t *adc_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0, uint32_t *pt_trigger1);
extern void trigger_freq_digital(uint32_t *adc_buffer, float sample_rate, float mean,
        uint32_t max_v, uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0);

extern bool trigger_freq(uint32_t *adc_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *freq, float *period, uint32_t *trigger0, uint32_t *trigger1);