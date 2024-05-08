#include <Arduino.h>

#include "algo/waveform.hpp"


#define DAC_PIN A0 // Make code a bit more legible


void test_sin() {
	for (int i = 0; i < 32; i++) {
		double val = sin(M_PI / 16.0 * i);
		Serial.println(String(val, 8));
	}
}

void setup() 
{
	Serial.begin(115200);

	// delay(3000);
	// test_sin();

	analogWriteResolution(DAC_RESOL_BITS); // Set analog out resolution to max, 10-bits

	Wave.init(MAX_OUTPUT_VOLT);
}

void loop() 
{
	// Generate a voltage between 0 and 3.3V.
	// 0= 0V, 1023=3.3V, 512=1.65V, etc.
	analogWrite(DAC_PIN, Wave.GetDacVal());
}