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
	pinMode(UART2_RX, INPUT);
	pinMode(UART2_TX, OUTPUT);

	DEV_Digital_Write(PWR_EN_PIN, 0);
}

/**
 * Module Power Control
 * Assumes powerOn and powerDown are macros or functions to control power.
 * Replace with actual implementation as needed.
 */
void module_power() {
	// Example implementation:
	DEV_Digital_Write(PWR_EN_PIN, 1); // Power On
	DEV_Delay_ms(2500);
	DEV_Digital_Write(PWR_EN_PIN, 0); // Power Down
}

/**
 * Module Initialization
 */
bool DEV_Module_Init(void) {
	// Initialize GPIO
	DEV_GPIO_Init();

	// Initialize Serial2 for UART communication
	Serial2.begin(UART_BAUD_RATE, SERIAL_8N1, UART2_RX, UART2_TX);
	Serial.println("Serial2 Initialized");

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
	static char b[500];
	memset(b, 0, sizeof(b));

	Serial.printf("CMD: %s\r\n", str);
	Serial2.println(str);

	unsigned long startTime = millis();
	String response = "";

	while (millis() - startTime < timeout) {
		while (Serial2.available()) {
			// char c = Serial2.read();
			// response += c;
			b[i++] = Serial2.read();
		}
	}
	if (strstr(b, back) == NULL)
	{
		Serial.printf("%s back: %s\r\n", str, b);
		return 0;
	}
	else
	{
		Serial.printf("%s\r\n", b);
		return 1;
	}
	printf("\r\n");
	// Serial2.print(str);
	// Serial2.print(" back: ");
	// Serial2.println(response);

	// return response.indexOf(back) != -1;
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
	Serial2.println(str);

	unsigned long startTime = millis();
	String response = "";

	while (millis() - startTime < timeout) {
		while (Serial2.available()) {
			// char c = Serial2.read();
			// response += c;
			b[i++] = Serial2.read();
		}
	}

	if (strstr(b, back) == NULL)
	{
		Serial.printf("%s back:\t %s\r\n", str, b);
	}
	else
	{
		Serial.printf("%s \r\n", b);
	}
	Serial.printf("Response information is: %s\r\n", b);
	return b;

	// if (response.indexOf(back) == -1) {
	// 	Serial2.print(str);
	// 	Serial2.print(" back:\t ");
	// 	Serial2.println(response);
	// } else {
	// 	Serial2.println(response);
	// }

	// static char respBuffer[500];
	// response.toCharArray(respBuffer, sizeof(respBuffer));
	// Serial2.print("Response information is: ");
	// Serial2.println(respBuffer);
	// return respBuffer;
}

/**
 * Send Command and Wait for AT Response
 */
bool sendCMD_waitResp_AT(const char *str, const char *back, int timeout) {
	uint16_t i = 0;
	uint64_t t = 0;
	static char b[114];
	memset(b, 0, sizeof(b));

	Serial.printf("CMD: %s\r\n", str);
	Serial2.println(str);

	unsigned long startTime = millis();
	String response = "";

	while (millis() - startTime < timeout) {
		while (Serial2.available()) {
			// char c = Serial2.read();
			// response += c;
			b[i++] = Serial2.read();
		}
	}

	if (strstr(b, back) == NULL)
	{
		Serial.printf("%s back: %s\r\n", str, b);
		return 0;
	}
	else
	{
		if (strstr(b, "CNACT?") == NULL)
		{
			Serial.printf("%s\r\n", b);
		}
		else
		{
			for (int i = 0; i < sizeof(b); i++)
			{
				Serial.printf("%c", b[i]);
			}
			Serial.printf("\r\n");
		}

		return 1;
	}
	Serial.printf("\r\n");

	// if (response.indexOf(back) == -1) {
	// 	Serial2.print(str);
	// 	Serial2.print(" back: ");
	// 	Serial2.println(response);
	// 	return false;
	// } else {
	// 	if (response.indexOf("CNACT?") == -1) {
	// 		Serial2.println(response);
	// 	} else {
	// 		for (int i = 0; i < response.length(); i++) {
	// 			Serial2.print(response[i]);
	// 		}
	// 		Serial2.println();
	// 	}
	// 	return true;
	// }
}

/**
 * Module Exit (No operation needed for Arduino)
 */
void DEV_Module_Exit(void) {
	// Typically, nothing is required here for Arduino
}


void genericInit() {
	DEV_Module_Init();
	// led_blink();
	DEV_Delay_ms(5000);
	check_start();
}

void genericNetInit() {
	set_network();
	check_network();
}