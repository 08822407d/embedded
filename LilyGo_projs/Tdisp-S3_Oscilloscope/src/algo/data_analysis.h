#ifndef _DATA_ANALYSIS_H_
#define _DATA_ANALYSIS_H_

	#include "../drv/adc.h"

	/* data_analysis.cpp */
	extern void peak_mean(SignalInfo *Wave);
	extern void digital_analog(SignalInfo *Wave);
	extern void trigger_freq_analog(SignalInfo *Wave);
	extern void trigger_freq_digital(SignalInfo *Wave);

	extern void trigger_freq(SignalInfo *Wave);

#endif /* _DATA_ANALYSIS_H_ */