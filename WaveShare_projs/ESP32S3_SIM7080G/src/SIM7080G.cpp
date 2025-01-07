/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * @file i2c_mode.ino
 * @brief Unit QRCode I2C Mode Example
 * @version 0.2
 * @date 2024-06-19
 *
 *
 * @Hardwares: M5Core + Unit QRCode
 * @Platform Version: Arduino M5Stack Board Manager v2.1.0
 * @Dependent Library:
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5UnitQRCode: https://github.com/m5stack/M5Unit-QRCode
 */
#include <Arduino.h>
#include <Wire.h>

#include "DEV_Config.hpp"


void setup() {
	Serial.begin(115200);
	delay(1000);

	genericInit();
	// genericNetInit();
}

void loop() {
	Serial.printf("test loop ...\n");

	GPSTest();
	// mqttTest();
}
