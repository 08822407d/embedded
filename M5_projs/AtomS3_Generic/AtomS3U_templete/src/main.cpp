/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */
/*
 * @Hardwares: M5Core + Mini Unit OLED
 * @Platform Version: Arduino M5Stack Board Manager v2.1.3
 * @Dependent Library:
 * M5Stack@^0.4.6: https://github.com/m5stack/M5Stack
 * U8g2_Arduino: https://github.com/olikraus/U8g2_Arduino
 */

#include <Arduino.h>
#include <Wire.h>
#include <ESP32Console.h>

using namespace ESP32Console;

Console console;

void setup(void)
{
	Serial.begin(115200);

	// console.registerCoreCommands();
	// console.registerSystemCommands();
	// console.registerGPIOCommands();
	// console.registerVFSCommands();
	// console.registerNetworkCommands();
}

void loop(void)
{
	delay(1000);
}