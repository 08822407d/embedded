#include "headers.h"


void peak_mean(uint16_t *adc_buffer, uint32_t len,
		float *max_value, float *min_value, float *pt_mean) {
	*max_value = adc_buffer[0];
	*min_value = adc_buffer[0];
	mean_filter filter(5);
	filter.init(adc_buffer[0]);

	float mean = 0;
	for (uint32_t i = 1; i < len; i++) {

		float value = filter.filter((float)adc_buffer[i]);
		if (value > *max_value)
			*max_value = value;
		if (value < *min_value)
			*min_value = value;

		mean += adc_buffer[i];
	}
	mean /= float(BUFF_SIZE);
	mean = to_voltage(mean);
	*pt_mean = mean;
}


//true if digital/ false if analog
bool digital_analog(uint16_t *adc_buffer, uint32_t max_v, uint32_t min_v) {
	uint32_t upper_threshold = max_v - 0.05 * (max_v - min_v);
	uint32_t lower_threshold = min_v + 0.05 * (max_v - min_v);
	uint32_t digital_data = 0;
	uint32_t analog_data = 0;
	for (uint32_t i = 0; i < BUFF_SIZE; i++) {
		if (adc_buffer[i] > lower_threshold) {
			if (adc_buffer[i] > upper_threshold) {
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
		return true;

	return false;
}


void trigger_freq_analog(uint16_t *adc_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0, uint32_t *pt_trigger1) {

	float positive_phase = 0;
	float negative_phase = 0;
	float period = 0;
	float freq = 0;
	bool signal_side = false;
	uint32_t trigger_count = 0;
	uint32_t trigger_num = 10;
	uint32_t trigger_temp[trigger_num] = {0};
	uint32_t trigger_index = 0;

	//get initial signal relative to the mean
	if (to_voltage(adc_buffer[0]) > mean)
		signal_side = true;

	//waveform repetitions calculation + get triggers time
	uint32_t wave_center = (max_v + min_v) / 2;
	for (uint32_t i = 1 ; i < BUFF_SIZE; i++) {
		if (signal_side && adc_buffer[i] < wave_center - (wave_center - min_v) * 0.2) {
			signal_side = false;
			negative_phase++;
		} else if (!signal_side && adc_buffer[i] > wave_center + (max_v - wave_center) * 0.2) {
			positive_phase++;
			if (trigger_count < trigger_num) {
				trigger_temp[trigger_count] = i;
				trigger_count++;
			}
			signal_side = true;
		}
	}

	//frequency calculation
	if (trigger_count < 2) {
		trigger_temp[0] = 0;
		trigger_index = 0;
		positive_phase = 0;
		period = 0;
	} else {
		// period = total time / triggers
		// in seconds
		period = (BUFF_SIZE / sample_rate) /
					((positive_phase + negative_phase) / 2);
		
		freq = 1 / period;

		// //from 2000 to 80 hz -> uses mean of the periods for precision
		// if (positive_phase < 2000 && positive_phase > 80) {
		// 	period = 0;
		// 	for (uint32_t i = 1; i < trigger_count; i++) {
		// 		period += trigger_temp[i] - trigger_temp[i - 1];
		// 	}
		// 	period /= (trigger_count - 1);
		// 	positive_phase = sample_rate * 1000 / period;
		// } else if (trigger_count > 1 && positive_phase <= 80) {
		// //under 80hz, single period for frequency calculation
		// 	period = trigger_temp[1] - trigger_temp[0];
		// 	positive_phase = sample_rate * 1000 / period;
		// }
	}

	//setting triggers offset and getting second trigger for debug cursor on drawn_channel1
	/*
		 The trigger function uses a rise porcentage (5%) obove the mean, thus,
		 the real waveform starting point is some datapoints back.
		 The resulting trigger gets a negative offset of 5% of the calculated period
	*/
	uint32_t trigger2 = 0;
	if (trigger_temp[0] - period * 0.05 > 0 && trigger_count > 1) {
		trigger_index = trigger_temp[0] - period * 0.05;
		trigger2 = trigger_temp[1] - period * 0.05;
	} else if (trigger_count > 2) {
		trigger_index = trigger_temp[1] - period * 0.05;
		if (trigger_count > 2)
			trigger2 = trigger_temp[2] - period * 0.05;
	}

	*pt_trigger0 = trigger_index;
	*pt_trigger1 = trigger2;
	*pt_freq = freq;
	*pt_period = period;
}


void trigger_freq_digital(uint16_t *adc_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *pt_freq, float *pt_period, uint32_t *pt_trigger0) {

	float freq = 0;
	float period = 0;
	bool signal_side = false;
	uint32_t trigger_count = 0;
	uint32_t trigger_num = 10;
	uint32_t trigger_temp[trigger_num] = {0};
	uint32_t trigger_index = 0;

	//get initial signal relative to the mean
	if (to_voltage(adc_buffer[0]) > mean) {
		signal_side = true;
	}

	//waveform repetitions calculation + get triggers time
	uint32_t wave_center = (max_v + min_v) / 2;
	bool normal_high = (mean > to_voltage(wave_center)) ? true : false;
	if (max_v - min_v > 4095 * (0.4 / 3.3)) {
		for (uint32_t i = 1 ; i < BUFF_SIZE; i++) {
			if (signal_side && adc_buffer[i] < wave_center - (wave_center - min_v) * 0.2) {
				//signal was high, fell -> trigger if normal high
				if (trigger_count < trigger_num && normal_high) {
					trigger_temp[trigger_count] = i;
					trigger_count++;
				}
				signal_side = false;
			} else if (!signal_side && adc_buffer[i] > wave_center + (max_v - wave_center) * 0.2) {
				freq++;
				//signal was low, rose -> trigger if normal low
				if (trigger_count < trigger_num && !normal_high) {
					trigger_temp[trigger_count] = i;
					trigger_count++;
				}
				signal_side = true;
			}
		}

		freq = freq * 1000 / 50;
		period = (float)(sample_rate * 1000.0) / freq; //us

		if (trigger_count > 1) {
			//from 2000 to 80 hz -> uses mean of the periods for precision
			if (freq < 2000 && freq > 80) {
				period = 0;
				for (uint32_t i = 1; i < trigger_count; i++) {
					period += trigger_temp[i] - trigger_temp[i - 1];
				}
				period /= (trigger_count - 1);
				freq = sample_rate * 1000 / period;
			} else if (trigger_count > 1 && freq <= 80) {
			//under 80hz, single period for frequency calculation
				period = trigger_temp[1] - trigger_temp[0];
				freq = sample_rate * 1000 / period;
			}
		}
		
		trigger_index = trigger_temp[0];

		if (trigger_index > 10)
			trigger_index -= 10;
		else
			trigger_index = 0;
	}

	*pt_trigger0 = trigger_index;
	*pt_freq = freq;
	*pt_period = period;
}


bool trigger_freq(uint16_t *adc_buffer, float sample_rate, float mean, uint32_t max_v,
		uint32_t min_v, float *freq, float *period, uint32_t *trigger0, uint32_t *trigger1) {
	//if analog mode OR auto mode and wave recognized as analog
	bool digital_data = digital_analog(adc_buffer, max_v, min_v);
	if (digital_wave_option == 1 || (digital_wave_option == 0 && !digital_data))
		trigger_freq_analog(adc_buffer, sample_rate, mean, max_v, min_v, freq, period, trigger0, trigger1);
	else
		trigger_freq_digital(adc_buffer, sample_rate, mean, max_v, min_v, freq, period, trigger0);
	
	return digital_data;
}