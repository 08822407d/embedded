#pragma once

#include <cstdint>
#include <cstring>
#include <cmath>

#include "config.h"
#include "consts.h"


#define SINE_INPUT_STEP(i) (((float)(i) / WAVEFORM_BUFSIZE) * M_PI)


class WaveForm {
public:
	int		Buff_idx = 0;
	uint16_t Buffer[WAVEFORM_BUFSIZE];


	WaveForm() {
		memset(Buffer, 0, sizeof(Buffer));
		FillSine(MAX_VOLT);
	}

	void FillSine(float max_volt) {
		for (int i = 0; i < WAVEFORM_BUFSIZE; i++)
			Buffer[i] = (int16_t)(sin(SINE_INPUT_STEP(i)) *
							VOLT_TO_DAC_VALUE(max_volt));
	}

	uint16_t GetDacVal() {
		if (Buff_idx >= WAVEFORM_BUFSIZE) Buff_idx = 0;
		return Buffer[Buff_idx++];
	}



private:
};



extern WaveForm Wave;