#include "DEV_Config.hpp"


/**
 * GPIO Write
 */
void DEV_Digital_Write(UWORD Pin, UBYTE Value) {
	digitalWrite(Pin, Value ? HIGH : LOW);
}

/**
 * GPIO Read
 */
UBYTE DEV_Digital_Read(UWORD Pin) {
	return digitalRead(Pin) == HIGH ? 1 : 0;
}


/**
 * Delay in milliseconds
 */
void DEV_Delay_ms(UDOUBLE xms) {
	delay(xms);
}

/**
 * Delay in microseconds
 */
void DEV_Delay_us(UDOUBLE xus) {
	delayMicroseconds(xus);
}


/**
 * GPIO Initialization
 */
void DEV_GPIO_Init(void) {
	pinMode(PWR_EN_PIN, OUTPUT);
	pinMode(MOD_WAKEUP_PIN, OUTPUT);
	// pinMode(UART2_RX, INPUT);
	// pinMode(UART2_TX, OUTPUT);
}

/**
 * Module Power Control
 * Assumes powerOn and powerDown are macros or functions to control power.
 * Replace with actual implementation as needed.
 */
void module_power(UBYTE state) {
	// Example implementation:
	DEV_Digital_Write(PWR_EN_PIN, state); // Power On
	DEV_Delay_ms(1000);
}

void module_wakeup() {
	DEV_Digital_Write(MOD_WAKEUP_PIN, HIGH);
	delay(1000);
	DEV_Digital_Write(MOD_WAKEUP_PIN, LOW);
}

/**
 * Module Initialization
 */
bool DEV_Module_Init(void) {
	// Initialize GPIO
	DEV_GPIO_Init();

	Serial.println("DEV_Module_Init	Starting...");
	module_power_on();
	module_wakeup();
	module_power_off();

	// Initialize ADC if needed
	// Note: Arduino initializes ADC automatically. Use analogRead() as needed.
	Serial.println("DEV_Module_Init OK");

	return true;
}

/**
 * Hex String to Byte Array Conversion
 */
void Hexstr_To_str(const char *source, unsigned char *dest, int sourceLen) {
	for (int i = 0; i < sourceLen; i += 2) {
		unsigned char highByte = toupper(source[i]);
		unsigned char lowByte = toupper(source[i + 1]);

		if (highByte > '9')
			highByte -= 0x37; // 'A'->10, 'F'->15
		else
			highByte -= 0x30; // '0'->0, '9'->9

		if (lowByte > '9')
			lowByte -= 0x37;
		else
			lowByte -= 0x30;

		dest[i / 2] = (highByte << 4) | lowByte;
	}
}

/**
 * Send Command and Wait for Response
 */
bool sendCMD_waitResp(const char *str, const char *back, int timeout) {
	uint16_t i = 0;
	uint64_t t = 0;
	char b[500];
	memset(b, 0, sizeof(b));

	Serial.printf("CMD: %s\r\n", str);
	ModuleSerial.println(str);

	unsigned long startTime = millis();
	String response = "";

	while (millis() - startTime < timeout) {
		while (ModuleSerial.available()) {
			b[i++] = ModuleSerial.read();
		}
	}

	if (strstr(b, back) == NULL) {
		Serial.printf("%s back: %s\r\n", str, b);
		return false;
	} else {
		Serial.printf("%s\r\n", b);
		return true;
	}
}

/**
 * Send Command and Wait for Response, Return Response String
 */
char* waitResp(const char *str, const char *back, int timeout) {
	uint16_t i = 0;
	uint64_t t = 0;
	static char b[500];
	memset(b, 0, sizeof(b));

	Serial.printf("CMD: %s\r\n", str);
	ModuleSerial.println(str);

	unsigned long startTime = millis();
	String response = "";

	while (millis() - startTime < timeout) {
		while (ModuleSerial.available()) {
			b[i++] = ModuleSerial.read();
		}
	}

	if (strstr(b, back) == NULL) {
		Serial.printf("%s back:\t %s\r\n", str, b);
	} else {
		Serial.printf("%s \r\n", b);
	}
	Serial.printf("Response information is: %s\r\n", b);
	return b;
}

/**
 * Send Command and Wait for AT Response
 */
bool sendCMD_waitResp_AT(const char *str, const char *back, int timeout) {
	uint16_t i = 0;
	uint64_t t = 0;
	char b[114];
	memset(b, 0, sizeof(b));

	Serial.printf("CMD: %s\r\n", str);
	ModuleSerial.println(str);

	unsigned long startTime = millis();
	String response = "";

	while (millis() - startTime < timeout) {
		while (ModuleSerial.available()) {
			b[i++] = ModuleSerial.read();
		}
	}

	if (strstr(b, back) == NULL) {
		Serial.printf("%s back: %s\r\n", str, b);
		return false;
	} else {
		if (strstr(b, "CNACT?") == NULL) {
			Serial.printf("%s\r\n", b);
		} else {
			for (int i = 0; i < sizeof(b); i++) {
				Serial.printf("%c", b[i]);
			}
			Serial.printf("\r\n");
		}
		return true;
	}
}

/**
 * Module Exit (No operation needed for Arduino)
 */
void DEV_Module_Exit(void) {
	// Typically, nothing is required here for Arduino
}


void genericInit() {
	DEV_Module_Init();
	check_start();
}

void genericNetInit() {
	check_network();
}