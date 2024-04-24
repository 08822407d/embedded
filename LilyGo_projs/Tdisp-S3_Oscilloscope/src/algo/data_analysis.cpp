#include "headers.h"


#define TRIG_SCALE		0.2
#define TRIG_MAX_COUNT	10

void peak_mean(SignalInfo *Wave) {
	Wave->MaxVal = Wave->SampleBuff[0];
	Wave->MinVal = Wave->SampleBuff[0];
	mean_filter filter(5);
	filter.init(Wave->SampleBuff[0]);

	float mean = 0;
	for (uint32_t i = 1; i < Wave->SampleNum; i++) {
		float value = filter.filter((float)Wave->SampleBuff[i]);
		if (value > Wave->MaxVal)
			Wave->MaxVal = value;
		if (value < Wave->MinVal)
			Wave->MinVal = value;

		mean += Wave->SampleBuff[i];
	}
	mean /= float(Wave->SampleNum);
	Wave->MeanVolt = to_voltage(mean);
}


//true if digital/ false if analog
void digital_analog(SignalInfo *Wave) {
	uint32_t upper_threshold =
		Wave->MaxVal - 0.05 * (Wave->MaxVal - Wave->MinVal);
	uint32_t lower_threshold =
		Wave->MinVal + 0.05 * (Wave->MaxVal - Wave->MinVal);

	uint32_t digital_data = 0;
	uint32_t analog_data = 0;
	for (uint32_t i = 0; i < Wave->SampleNum; i++) {
		if (Wave->SampleBuff[i] > lower_threshold) {
			if (Wave->SampleBuff[i] > upper_threshold) {
				//HIGH DIGITAL
				digital_data++;
			}
			else {
				//ANALOG/TRANSITION
				analog_data++;
			}
		}
		else {
			//LOW DIGITAL
			digital_data++;
		}
	}

	//more than 50% of data is analog
	if (analog_data < digital_data)
		Wave->IsDigital = true;
	else
		Wave->IsDigital = false;
}


void trigger_freq_analog(SignalInfo *Wave) {
	uint32_t *adc_buffer = Wave->SampleBuff;
	float period = 0;
	bool signal_side = false;
	uint32_t trigger_count = 0;
	uint32_t trigger_indices[TRIG_MAX_COUNT] = {0};
	uint32_t trigger_index = 0;
	uint32_t trigger2 = 0;

	//get initial signal relative to the mean
	if (to_voltage(adc_buffer[0]) > Wave->MeanVolt)
		signal_side = true;

	// waveform repetitions calculation + get triggers time
	uint32_t wave_center = (Wave->MaxVal + Wave->MinVal) / 2;
	for (uint32_t i = 1 ; i < Wave->SampleNum; i++) {
		if (signal_side && adc_buffer[i] <
				wave_center - (wave_center - Wave->MinVal) * TRIG_SCALE) {
			signal_side = false;
		} else if (!signal_side && adc_buffer[i] >
				wave_center + (Wave->MaxVal - wave_center) * TRIG_SCALE) {
			if (trigger_count < TRIG_MAX_COUNT)
				trigger_indices[trigger_count++] = i;
		
			signal_side = true;
		}
	}

	//frequency calculation
	if (trigger_count < 2) {
		trigger_indices[0] = 0;
		trigger_count = 0;
	} else {
		uint one_period_samples =
				(trigger_indices[trigger_count - 1] - trigger_indices[0]) /
					(trigger_count - 1);
		period = (float)one_period_samples / Wave->SampleRate;
	}
	if (period == 0 && trigger_count < 2)
		return;

	// /*
	// 	 The trigger function uses a rise porcentage (5%) obove the mean, thus,
	// 	 the real waveform starting point is some datapoints back.
	// 	 The resulting trigger gets a negative offset of 5% of the calculated period
	// */
	// if (trigger_indices[0] - period * TRIG_SCALE > 0 && trigger_count > 1) {
	// 	trigger_index = trigger_indices[0] - period * TRIG_SCALE;
	// 	trigger2 = trigger_indices[1] - period * TRIG_SCALE;
	// } else if (trigger_count > 2) {
	// 	trigger_index = trigger_indices[1] - period * TRIG_SCALE;
	// 	if (trigger_count > 2)
	// 		trigger2 = trigger_indices[2] - period * TRIG_SCALE;
	// }
	for (int i = trigger_indices[1]; i >= 0; i--)
		if ((trigger_index = trigger_indices[1]) == wave_center)
			break;
	trigger2 = trigger_indices[2] - period * TRIG_SCALE;

	Wave->TrigIdx_0 = trigger_index;
	Wave->TrigIdx_1 = trigger2;
	Wave->Period = period;
	Wave->Freq = 1.0 / period;
}


void trigger_freq_digital(SignalInfo *Wave) {
	uint32_t *adc_buffer = Wave->SampleBuff;
	float freq = 0;
	float period = 0;
	bool signal_side = false;
	uint32_t trigger_count = 0;
	uint32_t trigger_num = 10;
	uint32_t trigger_temp[trigger_num] = {0};
	uint32_t trigger_index = 0;

	//get initial signal relative to the mean
	if (to_voltage(adc_buffer[0]) > Wave->MeanVolt) {
		signal_side = true;
	}

	//waveform repetitions calculation + get triggers time
	uint32_t wave_center = (Wave->MaxVal + Wave->MinVal) / 2;
	bool normal_high = (Wave->MeanVolt > to_voltage(wave_center)) ? true : false;
	if (Wave->MaxVal - Wave->MinVal > 4095 * (0.4 / 3.3)) {
		for (uint32_t i = 1 ; i < SAMPLE_BUFFLEN; i++) {
			if (signal_side && adc_buffer[i] < wave_center - (wave_center - Wave->MinVal) * 0.2) {
				//signal was high, fell -> trigger if normal high
				if (trigger_count < trigger_num && normal_high) {
					trigger_temp[trigger_count] = i;
					trigger_count++;
				}
				signal_side = false;
			} else if (!signal_side && adc_buffer[i] > wave_center + (Wave->MaxVal - wave_center) * 0.2) {
				freq++;
				//signal was low, rose -> trigger if normal low
				if (trigger_count < trigger_num && !normal_high) {
					trigger_temp[trigger_count] = i;
					trigger_count++;
				}
				signal_side = true;
			}
		}

		freq = freq * 1000.0 / 50;
		period = (float)(Wave->SampleRate * 1000.0) / freq; //us

		if (trigger_count > 1) {
			//from 2000 to 80 hz -> uses mean of the periods for precision
			if (freq < 2000 && freq > 80) {
				period = 0;
				for (uint32_t i = 1; i < trigger_count; i++) {
					period += trigger_temp[i] - trigger_temp[i - 1];
				}
				period /= (trigger_count - 1);
				freq = Wave->SampleRate * 1000.0 / period;
			} else if (trigger_count > 1 && freq <= 80) {
			//under 80hz, single period for frequency calculation
				period = trigger_temp[1] - trigger_temp[0];
				freq = Wave->SampleRate * 1000.0 / period;
			}
		}
		
		trigger_index = trigger_temp[0];

		if (trigger_index > 10)
			trigger_index -= 10;
		else
			trigger_index = 0;
	}

	Wave->TrigIdx_0 = trigger_index;
	Wave->Freq = freq;
	Wave->Freq = period;
}


void trigger_freq(SignalInfo *Wave) {
	//if analog mode OR auto mode and wave recognized as analog
	digital_analog(Wave);
	if (digital_wave_option == 1 ||
			(digital_wave_option == 0 && !Wave->IsDigital))
		trigger_freq_analog(Wave);
	else
		trigger_freq_digital(Wave);
}