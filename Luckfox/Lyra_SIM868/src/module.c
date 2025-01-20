#include "gpio.h"
#include "serial.h"
#include "utils_defs.h"
#include "module.h"


// /**
//  * Send Command and Wait for Response
//  */
// bool sendCMD_waitResp(const char *str, const char *back, int timeout) {
// 	uint16_t i = 0;
// 	uint64_t t = 0;
// 	static char b[500];
// 	memset(b, 0, sizeof(b));

// 	Serial.printf("CMD: %s\r\n", str);
// 	Serial2.println(str);

// 	unsigned long startTime = millis();
// 	String response = "";

// 	while (millis() - startTime < timeout) {
// 		while (Serial2.available()) {
// 			b[i++] = Serial2.read();
// 		}
// 	}

// 	if (strstr(b, back) == NULL) {
// 		Serial.printf("%s back: %s\r\n", str, b);
// 		return false;
// 	} else {
// 		Serial.printf("%s\r\n", b);
// 		return true;
// 	}
// }

// /**
//  * Send Command and Wait for Response, Return Response String
//  */
// char* waitResp(const char *str, const char *back, int timeout) {
// 	uint16_t i = 0;
// 	uint64_t t = 0;
// 	static char b[500];
// 	memset(b, 0, sizeof(b));

// 	Serial.printf("CMD: %s\r\n", str);
// 	Serial2.println(str);

// 	unsigned long startTime = millis();
// 	String response = "";

// 	while (millis() - startTime < timeout) {
// 		while (Serial2.available()) {
// 			b[i++] = Serial2.read();
// 		}
// 	}

// 	if (strstr(b, back) == NULL) {
// 		Serial.printf("%s back:\t %s\r\n", str, b);
// 	} else {
// 		Serial.printf("%s \r\n", b);
// 	}
// 	Serial.printf("Response information is: %s\r\n", b);
// 	return b;
// }

// /**
//  * Send Command and Wait for AT Response
//  */
// bool sendCMD_waitResp_AT(const char *str, const char *back, int timeout) {
// 	uint16_t i = 0;
// 	uint64_t t = 0;
// 	static char b[114];
// 	memset(b, 0, sizeof(b));

// 	Serial.printf("CMD: %s\r\n", str);
// 	Serial2.println(str);

// 	unsigned long startTime = millis();
// 	String response = "";

// 	while (millis() - startTime < timeout) {
// 		while (Serial2.available()) {
// 			b[i++] = Serial2.read();
// 		}
// 	}

// 	if (strstr(b, back) == NULL) {
// 		Serial.printf("%s back: %s\r\n", str, b);
// 		return false;
// 	} else {
// 		if (strstr(b, "CNACT?") == NULL) {
// 			Serial.printf("%s\r\n", b);
// 		} else {
// 			for (int i = 0; i < sizeof(b); i++) {
// 				Serial.printf("%c", b[i]);
// 			}
// 			Serial.printf("\r\n");
// 		}
// 		return true;
// 	}
// }


/**
 * GPIO Initialization
 */
int DEV_GPIO_Init(void) {
	// 导出sim868模组电源gpio和唤醒gpio
	if (pinExport(SIM868_PWR_PIN) == -1) {
		perror("Failed to Export SIM868_PWR_pin\n");
		return -1;
	}
	if (pinExport(SIM868_WAKE_PIN) == -1) {
		perror("Failed to Export SIM868_WAKE_pin\n");
		return -1;
	}
	// 设置引脚为输出
	if (pinMode(SIM868_PWR_PIN, GPIO_OUT) == -1) {
		perror("Failed to set SIM868_WAKE_pin pinMode\n");
		return -1;
	}
	if (pinMode(SIM868_WAKE_PIN, GPIO_OUT) == -1) {
		perror("Failed to set SIM868_WAKE_pin pinMode\n");
		return -1;
	}
	// pinMode(UART2_RX, INPUT);
	// pinMode(UART2_TX, OUTPUT);


	digitalWrite(SIM868_PWR_PIN, DIGITAL_LOW);

	// digitalWrite(MOD_WAKEUP_PIN, HIGH);
	// delay(500);
	// digitalWrite(MOD_WAKEUP_PIN, LOW);
}

/**
 * Module Power Control
 * Assumes powerOn and powerDown are macros or functions to control power.
 * Replace with actual implementation as needed.
 */
void module_power() {
	// Example implementation:
	digitalWrite(SIM868_PWR_PIN, DIGITAL_HIGH); // Power On
	delay(2500);
	digitalWrite(SIM868_PWR_PIN, DIGITAL_LOW); // Power Down
}

void module_wakeup() {
	digitalWrite(SIM868_WAKE_PIN, DIGITAL_HIGH);
	delay(500);
	digitalWrite(SIM868_WAKE_PIN, DIGITAL_LOW);
}

/**
 * Module Initialization
 */
bool DEV_Module_Init(void) {
	// Initialize GPIO
	DEV_GPIO_Init();

	// Initialize Serial2 for UART communication
	int serial_fd = openSerial(0);
	configSerial(serial_fd);
	delay(1000);
	printf("Serial2 Initialized");

	// Initialize ADC if needed
	// Note: Arduino initializes ADC automatically. Use analogRead() as needed.
	printf("DEV_Module_Init OK");

	return true;
}


void genericInit() {
	DEV_Module_Init();
	delay(5000);
	check_start();
}

void genericNetInit() {
	// set_network();
	// check_network();
}