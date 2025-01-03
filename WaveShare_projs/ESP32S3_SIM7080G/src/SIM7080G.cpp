// /*
//  * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
//  *
//  * SPDX-License-Identifier: MIT
//  */

// /**
//  * @file i2c_mode.ino
//  * @brief Unit QRCode I2C Mode Example
//  * @version 0.2
//  * @date 2024-06-19
//  *
//  *
//  * @Hardwares: M5Core + Unit QRCode
//  * @Platform Version: Arduino M5Stack Board Manager v2.1.0
//  * @Dependent Library:
//  * M5Unified: https://github.com/m5stack/M5Unified
//  * M5GFX: https://github.com/m5stack/M5GFX
//  * M5UnitQRCode: https://github.com/m5stack/M5Unit-QRCode
//  */
#include <Arduino.h>
#include <Wire.h>

#define WAVESHARE_UART_TX	11
#define WAVESHARE_UART_RX	12

#define SIM_POWER_PIN		39
#define SIM_WAKEUP_PIN		41


extern void PowerOnSIM7080G(void);

void setup() {
	Serial1.begin(115200);
	Serial2.begin(115200, SERIAL_8N1, WAVESHARE_UART_RX, WAVESHARE_UART_TX);
	delay(500);

	PowerOnSIM7080G();
}

void loop() {
	// 检查Serial2是否有数据可用
	if (Serial2.available()) {
		// 如果有数据，就读取并转发到Serial1
		while (Serial2.available()) {
			char incomingByte = Serial2.read();
			Serial1.write(incomingByte);
		}
	}
}

/*************  ✨ Codeium Command ⭐  *************/
/**
 * @brief Powers on the SIM7080 module by setting the power and wakeup pins to HIGH.
 * 
 * This function configures the SIM_POWER_PIN and SIM_WAKEUP_PIN as output pins,
 * then sets them to HIGH to power on the SIM7080 module. It includes delays to
 * ensure the pins are properly set before and after powering on the module.
 */

/******  a114e62c-2c28-4ee0-9aa4-e629420973a2  *******/
void PowerOnSIM7080G() {
	pinMode(SIM_POWER_PIN, OUTPUT);
	pinMode(SIM_WAKEUP_PIN, OUTPUT);
	delay(500);

	digitalWrite(SIM_POWER_PIN, HIGH);
	digitalWrite(SIM_WAKEUP_PIN, HIGH);
	delay(1000);
}