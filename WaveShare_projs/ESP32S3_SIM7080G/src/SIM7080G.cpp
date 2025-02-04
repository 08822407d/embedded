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


// void setup() {
// 	Serial.begin(115200);
// 	delay(1000);

// 	genericInit();
// 	// genericNetInit();
// }

// void loop() {
// 	Serial.printf("test loop ...\n");

// 	GPSTest();
// 	// mqttTest();
// }



// Echo 测试程序：主机端
#define RX_PIN 15
#define TX_PIN 16
// #define RX_PIN 41
// #define TX_PIN 42

HardwareSerial mySerial(1);

void setup() {
	// 初始化串口
	Serial.begin(115200); // 输出到USB串口监视器
	mySerial.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // 初始化 UART 1
}

void loop() {
	// 主机读取数据并发送到串口
	if (mySerial.available()) {
		String msg = mySerial.readString();
		Serial.print("Received from UART: ");
		Serial.println(msg);
		// 将数据回发到主机
		mySerial.println(msg);
	}

	// debug机发送的串口数据
	if (Serial.available()) {
		String msg = Serial.readString();
		Serial.print("Received from Debug: ");
		Serial.println(msg);
		mySerial.println(msg); // 发送数据到 UART
	}
}