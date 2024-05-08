#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>

#include "config.h"
#include "consts.h"


#define SINE_INPUT_STEP(i) (((float)(i) / WAVEFORM_BUFSIZE) * 2 * M_PI)


class WaveForm {
public:
	int		Buff_idx = 0;
	uint32_t Buffer[WAVEFORM_BUFSIZE];


	WaveForm() {
		init(MAX_OUTPUT_VOLT);
	}

	void init(float max_volt) {
		memset(Buffer, 0, sizeof(Buffer));
		FillSine(max_volt);
	}

	void FillSine(float max_volt) {
		for (int i = 0; i < WAVEFORM_BUFSIZE; i++)
			Buffer[i] = (int32_t)((sin(SINE_INPUT_STEP(i)) + 1.0) / 2 * VOLT_TO_DAC_VALUE(max_volt));
	}

	uint32_t GetDacVal() {
		if (Buff_idx >= WAVEFORM_BUFSIZE) Buff_idx = 0;
		return Buffer[Buff_idx++];
	}


private:
};



extern WaveForm Wave;