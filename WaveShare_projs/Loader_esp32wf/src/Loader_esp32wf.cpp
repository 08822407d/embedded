 /**
  ******************************************************************************
  * @file    Loader.h
  * @author  Waveshare Team
  * @version V1.0.0
  * @date    23-January-2018
  * @brief   The main file.
  *          This file provides firmware functions:
  *           + Initialization of Serial Port, SPI pins and server
  *           + Main loop
  *
  ******************************************************************************
*/ 

/* Includes ------------------------------------------------------------------*/
#include "srvr.h" // Server functions
#include "button.h"

/* Entry point ----------------------------------------------------------------*/
void setup() 
{
	// Serial port initialization
	Serial.begin(115200);
	  // initialize the pushbutton pin as an input:
	delay(10);

	// Server initialization
	Srvr__setup();

	// SPI initialization
	EPD_initSPI();

	InitUserButton();

	// Initialization is complete
	Serial.print("\r\nOk!\r\n");
}

/* The main loop -------------------------------------------------------------*/
void loop() 
{
	button.tick();
	// The server state observation
	Srvr__loop();
}
