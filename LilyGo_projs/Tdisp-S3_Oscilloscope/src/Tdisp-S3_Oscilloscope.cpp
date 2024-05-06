#include "headers.h"

// #include <Arduino_APA102.h>


// #define PIN_APA102_CLK		45
// #define PIN_APA102_DI		42
// Arduino_APA102 ledStrip = Arduino_APA102(8, PIN_APA102_DI, PIN_APA102_CLK);


TaskHandle_t	task_menu;
TaskHandle_t	task_adc;

bool stop				= false;
bool stop_change		= false;
bool updating_screen	= false;
bool new_data			= false;


void setup() {
	Serial.begin(115200);

	setup_screen();

	InitUserButton();

	config_adc();
	
	// ledStrip.begin();


	xTaskCreatePinnedToCore(
		core0_task,
		"menu_handle",
		16384,			/* Stack size in words */
		NULL,			/* Task input parameter */
		0,				/* Priority of the task */
		&task_menu,		/* Task handle. */
		0
	);				/* Core where the task should run */

	xTaskCreatePinnedToCore(
		core1_task,
		"adc_handle",
		16384,			/* Stack size in words */
		NULL,			/* Task input parameter */
		3,				/* Priority of the task */
		&task_adc,		/* Task handle. */
		1
	);				/* Core where the task should run */
}


void core0_task(void * pvParameters) {
	(void) pvParameters;
	for (;;) {
		menu_handler();
		if (new_data || GlobOpts.menu_action) {
			new_data = false;
			GlobOpts.menu_action = false;

			updating_screen = true;
			update_screen(&CurrentWave);
			updating_screen = false;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}

void core1_task(void * pvParameters) {
	(void) pvParameters;

	for (;;) {
		if (!single_trigger) {
			while (updating_screen)
				vTaskDelay(pdMS_TO_TICKS(1));

			if (!stop) {
				if (stop_change)
					stop_change = false;

				new_data = ADC_Sampling(&CurrentWave);
			} else {
				if (!stop_change)
					stop_change = true;
			}
			vTaskDelay(pdMS_TO_TICKS(1));
		} else {
			float old_mean = 0;
			while (single_trigger) {
				stop = true;

				//signal captured (pp > 0.4V || changing mean > 0.2V) -> DATA ANALYSIS
				if (ADC_Sampling(&CurrentWave) && 
						(old_mean != 0 && fabs(CurrentWave.MeanVolt - old_mean) > 0.2) ||
						to_voltage(CurrentWave.MaxVal) - to_voltage(CurrentWave.MinVal) > 0.05) {
					float freq = 0;
					float period = 0;
					uint32_t trigger0 = 0;
					uint32_t trigger1 = 0;

					//if analog mode OR auto mode and wave recognized as analog
					trigger_freq(&CurrentWave);

					single_trigger = false;
					new_data = true;
					// Serial.println("Single GOT");
					//return to normal execution in stop mode
				}
				vTaskDelay(pdMS_TO_TICKS(1));   //time for the other task to start (low priorit)
			}
			vTaskDelay(pdMS_TO_TICKS(250));
		}
	}
}


void loop() {
	BtnBack.tick();
	BtnEnter.tick();
	BtnEnter_AckLongPressTimer.tick();
	BtnBack_AckLongPressTimer.tick();
}
