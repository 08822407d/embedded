#pragma once

#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H

#include <Arduino.h>

#include "modules.hpp"

#define USING_SIM_MODULE	MODULE_SIM868

	extern HardwareSerial ModuleSerial;
	extern HardwareSerial MasterSerial;

	// Type definitions
	typedef uint16_t UWORD;
	typedef uint8_t UBYTE;
	typedef unsigned long UDOUBLE;

	// GPIO Pins (Define according to your hardware setup)
	// const int PWR_EN_PIN = 14;   // Example pin number for power enable
	// const int LED_EN_PIN = 25;   // Example pin number for LED enable
	const int PWR_EN_PIN		= 39;   // Example pin number for power enable
	const int MOD_WAKEUP_PIN	= 41;   // Example pin number for power enable

	// UART Configuration
	#define UART_BAUD_RATE		115200  // Set your desired baud rate
	#define UART_TIMEOUT_MS		1000   // Default timeout for UART operations

	// GPIO Mode Constants
	#define UART2_RX			12
	#define UART2_TX			11


	// Function Declarations

	// GPIO Functions
	void DEV_Digital_Write(UWORD Pin, UBYTE Value);
	UBYTE DEV_Digital_Read(UWORD Pin);
	void DEV_GPIO_Mode(UWORD Pin, UWORD Mode);
	void DEV_GPIO_Init(void);

	// Delay Functions
	void DEV_Delay_ms(UDOUBLE xms);
	void DEV_Delay_us(UDOUBLE xus);

	// LED Functions
	void led_blink();

	// Module Power Control
	void module_power();

	// Module Initialization and Exit
	bool DEV_Module_Init(void);
	void DEV_Module_Exit(void);

	// Hex String Conversion
	void Hexstr_To_str(const char *source, unsigned char *dest, int sourceLen);

	// UART Communication Functions
	bool sendCMD_waitResp(const char *str, const char *back, int timeout);
	char* waitResp(const char *str, const char *back, int timeout);
	bool sendCMD_waitResp_AT(const char *str, const char *back, int timeout);

	extern void genericInit(void);
	extern void genericNetInit(void);

#endif // DEV_CONFIG_H