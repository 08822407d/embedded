#include "headers.h"


TaskHandle_t	task_menu;
TaskHandle_t	task_adc;

float RATE				= 1000; //in ksps --> 1000 = 1Msps
bool stop				= false;
bool stop_change		= false;
bool updating_screen	= false;
bool new_data			= false;


void setup() {
	Serial.begin(115200);

	// delay(500);
	setup_screen();
	// delay(500);

	InitUserButton();
	// delay(500);

	characterize_adc();

	xTaskCreatePinnedToCore(
		core0_task,
		"menu_handle",
		10000,			/* Stack size in words */
		NULL,			/* Task input parameter */
		0,				/* Priority of the task */
		&task_menu,		/* Task handle. */
		0);				/* Core where the task should run */

	xTaskCreatePinnedToCore(
		core1_task,
		"adc_handle",
		10000,			/* Stack size in words */
		NULL,			/* Task input parameter */
		3,				/* Priority of the task */
		&task_adc,		/* Task handle. */
		1);				/* Core where the task should run */
}


void core0_task( void * pvParameters ) {
	(void) pvParameters;
	for (;;) {
		menu_handler();
		if (new_data || menu_action) {
			new_data = false;
			menu_action = false;

			updating_screen = true;
			update_screen(AdcSample_Buffer, RATE);
			updating_screen = false;
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void core1_task( void * pvParameters ) {
	(void) pvParameters;

	for (;;) {
		if (!single_trigger) {
			while (updating_screen)
				vTaskDelay(pdMS_TO_TICKS(1));

			if (!stop) {
				if (stop_change)
					stop_change = false;

				RATE = ADC_Sampling(AdcSample_Buffer);
				new_data = true;
			} else {
				if (!stop_change)
					stop_change = true;
			}
			vTaskDelay(pdMS_TO_TICKS(100));
		} else {
			float old_mean = 0;
			while (single_trigger) {
				stop = true;
				RATE = ADC_Sampling(AdcSample_Buffer);
				float mean = 0;
				float max_v, min_v;
				peak_mean(AdcSample_Buffer, BUFF_SIZE, &max_v, &min_v, &mean);

				//signal captured (pp > 0.4V || changing mean > 0.2V) -> DATA ANALYSIS
				if ((old_mean != 0 && fabs(mean - old_mean) > 0.2) || to_voltage(max_v) - to_voltage(min_v) > 0.05) {
					float freq = 0;
					float period = 0;
					uint32_t trigger0 = 0;
					uint32_t trigger1 = 0;

					//if analog mode OR auto mode and wave recognized as analog
					trigger_freq(AdcSample_Buffer, RATE, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);

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
