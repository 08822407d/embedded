/*
 * SPDX-FileCopyrightText: 2024 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include <Arduino.h>
#include <Wire.h>

#include "DEV_Config.hpp"

#define MODULE_RX_PIN 12
#define MODULE_TX_PIN 11
// Echo 测试程序：主机端
#define MASTER_RX_PIN 15
#define MASTER_TX_PIN 16

HardwareSerial ModuleSerial(1);
HardwareSerial MasterSerial(2);

void setup() {
	// 初始化串口
	Serial.begin(115200); // 输出到USB串口监视器
	ModuleSerial.begin(115200, SERIAL_8N1, MODULE_RX_PIN, MODULE_TX_PIN); // 初始化 UART 1
	MasterSerial.begin(115200, SERIAL_8N1, MASTER_RX_PIN, MASTER_TX_PIN); // 初始化 UART 1
	
	delay(1000);
	genericInit();
	genericNetInit();
	delay(1000);

	Serial.println("Start Repeat.");
}

void loop() {
	// // 主机读取数据并发送到串口
	// if (MasterSerial.available()) {
	// 	String msg = MasterSerial.readString();
	// 	Serial.print("Received from UART: ");
	// 	Serial.println(msg);
	// 	// 将数据回发到主机
	// 	MasterSerial.println(msg);
	// }

	// // debug机发送的串口数据
	// if (Serial.available()) {
	// 	String msg = Serial.readString();
	// 	Serial.print("Received from Debug: ");
	// 	Serial.println(msg);
	// 	MasterSerial.println(msg); // 发送数据到 UART
	// }

	if (Serial.available()) {
		String msg = Serial.readString();
		Serial.print("Received from Debug: ");
		Serial.println(msg);
		ModuleSerial.println(msg); // 发送数据到 UART
	}
	if (ModuleSerial.available()) {
		String msg = ModuleSerial.readString();
		Serial.print("Received from Module: ");
		Serial.println(msg);
	}
}