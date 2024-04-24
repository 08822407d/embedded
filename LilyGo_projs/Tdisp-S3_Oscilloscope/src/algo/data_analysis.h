#ifndef _DATA_ANALYSIS_H_
#define _DATA_ANALYSIS_H_

	class SignalInfo {
	public:
		float		MeanVolt;
		uint32_t	MaxVal;		// Raw Value from ADC, not true Voltage
		uint32_t	MinVal;		// Raw Value from ADC, not true Voltage
		uint32_t	Freq;
		uint32_t	Period;
		uint32_t	TrigIdx_0;
		uint32_t	TrigIdx_1;
	};



	extern SignalInfo CurrentWave;


	/* data_analysis.cpp */
	extern void peak_mean(uint32_t *adc_buffer, uint32_t len, SignalInfo *Wave);
	extern bool digital_analog(uint32_t *adc_buffer, SignalInfo *Wave);
	extern void trigger_freq_analog(uint32_t *adc_buffer, float sample_rate, SignalInfo *Wave);
	extern void trigger_freq_digital(uint32_t *adc_buffer, float sample_rate, SignalInfo *Wave);

	extern bool trigger_freq(uint32_t *adc_buffer, float sample_rate, SignalInfo *Wave);

#endif /* _DATA_ANALYSIS_H_ */