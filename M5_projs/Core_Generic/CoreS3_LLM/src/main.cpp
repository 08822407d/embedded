/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
#include <Arduino.h>
#include <M5Unified.h>
#include <M5ModuleLLM.h>
#include "m5_module_fan.hpp"


M5ModuleLLM module_llm;
M5ModuleLLM_VoiceAssistant voice_assistant(&module_llm);
M5ModuleFan moduleFan;

uint8_t deviceAddr = MODULE_FAN_BASE_ADDR;
uint8_t dutyCycle  = 80;

/* On ASR data callback */
void on_asr_data_input(String data, bool isFinish, int index)
{
	M5.Display.setTextColor(TFT_GREEN, TFT_BLACK);
	M5.Display.printf(">> %s\n", data.c_str());

	/* If ASR data is finish */
	if (isFinish) {
		M5.Display.setTextColor(TFT_YELLOW, TFT_BLACK);
		M5.Display.print(">> ");
	}
};

/* On LLM data callback */
void on_llm_data_input(String data, bool isFinish, int index)
{
	M5.Display.print(data);

	/* If LLM data is finish */
	if (isFinish) {
		M5.Display.print("\n");
	}
};

void setup()
{
	M5.begin();
	Serial.begin(115200);
	M5.Display.setTextSize(2);
	M5.Display.setTextScroll(true);

	while (!moduleFan.begin(&Wire1, deviceAddr, 21, 22, 400000)) {
		Serial.printf("Module FAN Init faile\r\n");
	}
	moduleFan.setStatus(MODULE_FAN_ENABLE);
	// Set the fan to rotate at 80% duty cycle
	moduleFan.setPWMDutyCycle(dutyCycle);


	/* Init module serial port */
	Serial2.begin(115200, SERIAL_8N1, 16, 17);  // Basic
	// Serial2.begin(115200, SERIAL_8N1, 13, 14);  // Core2
	// Serial2.begin(115200, SERIAL_8N1, 18, 17);  // CoreS3

	/* Init module */
	module_llm.begin(&Serial2);

	/* Make sure module is connected */
	M5.Display.printf(">> Check ModuleLLM connection..\n");
	while (1) {
		if (module_llm.checkConnection()) {
			break;
		}
	}

	/* Begin voice assistant preset */
	M5.Display.printf(">> Begin voice assistant..\n");
	int ret = voice_assistant.begin("HELLO");
	if (ret != MODULE_LLM_OK) {
		while (1) {
			M5.Display.setTextColor(TFT_RED);
			M5.Display.printf(">> Begin voice assistant failed\n");
		}
	}

	/* Register on ASR data callback function */
	voice_assistant.onAsrDataInput(on_asr_data_input);

	/* Register on LLM data callback function */
	voice_assistant.onLlmDataInput(on_llm_data_input);

	M5.Display.printf(">> Voice assistant ready\n");
}

void loop()
{
	/* Keep voice assistant preset update */
	voice_assistant.update();

	Serial.printf("\r\n");
	Serial.printf(" {\r\n");
	Serial.printf("    Work Status      : %d\r\n", moduleFan.getStatus());
	Serial.printf("    PWM  Frequency   : %d\r\n", moduleFan.getPWMFrequency());
	Serial.printf("    PWM  Duty Cycle  : %d\r\n", moduleFan.getPWMDutyCycle());
	Serial.printf("    RPM              : %d\r\n", moduleFan.getRPM());
	Serial.printf("    Signal Frequency : %d\r\n", moduleFan.getSignalFrequency());
	Serial.printf("    Firmware Version : %d\r\n", moduleFan.getFirmwareVersion());
	Serial.printf("         I2C Addrres : 0x%02X\r\n", moduleFan.getI2CAddress());
	Serial.printf("                             }\r\n");
	delay(500);
}