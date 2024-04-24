#ifndef _ADC_H_
#define _ADC_H_

    #include <driver/adc.h>
    #include <esp_adc_cal.h>
    #include <esp_wifi.h>


	class SignalInfo {
	public:
		float		MeanVolt;
		uint32_t	MaxVal;		// Raw Value from ADC, not true Voltage
		uint32_t	MinVal;		// Raw Value from ADC, not true Voltage
		uint32_t	Freq;
		uint32_t	Period;
		uint32_t	TrigIdx_0;
		uint32_t	TrigIdx_1;

		bool		IsDigital;
		uint32_t	SampleRate = ADC_SAMPLE_RATE;
		uint32_t	SampleNum;
		uint32_t	SampleBuff[SAMPLE_BUFFLEN];
	};



    /* adc.cpp */
	extern SignalInfo CurrentWave;
    extern void config_adc(void);
    extern float to_voltage(uint32_t reading);
    extern bool ADC_Sampling(SignalInfo *Wave);

#endif /* _ADC_H_ */