#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_wifi.h>


/* adc.cpp */
extern uint32_t AdcSample_Buffer[];
extern uint32_t SampleNum;
extern float mean, max_v, min_v;
extern void config_adc(void);
extern float to_voltage(float reading);
extern bool ADC_Sampling(uint32_t *AdcDataBuf);