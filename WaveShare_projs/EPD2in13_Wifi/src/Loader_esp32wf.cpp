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
#include <SPIFFS.h>

#include "srvr.h" // Server functions
#include "drv/button.h"


TaskHandle_t	task_server;
extern void core0_task(void * pvParameters);
// extern void core1_task(void * pvParameters);


/* Entry point ----------------------------------------------------------------*/
void setup() 
{
	// Serial port initialization
	Serial.begin(115200);
	// initialize the pushbutton pin as an input:
	delay(1000);

	SPIFFS.begin();

	// Server initialization
	Srvr__setup();

	// SPI initialization
	EPD_initSPI();

	InitUserButton();


	// xTaskCreatePinnedToCore(
	// 	core0_task,
	// 	"server_handle",
	// 	0x20000,			/* Stack size in words */
	// 	NULL,				/* Task input parameter */
	// 	0,					/* Priority of the task */
	// 	&task_server,		/* Task handle. */
	// 	0
	// );				/* Core where the task should run */

	// xTaskCreatePinnedToCore(
	// 	core1_task,
	// 	"menu_handle",
	// 	0x20000,			/* Stack size in words */
	// 	NULL,				/* Task input parameter */
	// 	0,					/* Priority of the task */
	// 	&task_server,		/* Task handle. */
	// 	0
	// );				/* Core where the task should run */

	// Initialization is complete
	Serial.print("\r\nOk!\r\n");

}

/* The main loop -------------------------------------------------------------*/
void loop() 
{
	UserBtn.tick();
	UserBtnAckLongPressTimer.tick();
	// The server state observation
	Srvr__loop();
}


void core0_task(void * pvParameters) {
	(void) pvParameters;
	for (;;) {
		Srvr__loop();
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void core1_task(void * pvParameters) {
	(void) pvParameters;
	for (;;) {
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}