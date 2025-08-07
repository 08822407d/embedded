#include "main.h"
#include "stm32u5xx_hal_gpio.h"
#include "stm32u5xx_it.h"
#include "app_freertos.h"

void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
		case EXTI_Key1_Pin: // Assuming GPIO_PIN_0 is connected to a button
			// Handle button press event
			HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); // Toggle an LED connected to GPIOB Pin 1
			break;
	}
}

void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
		case USER_BUTTON_Pin: // Assuming GPIO_PIN_0 is connected to a button
			// Handle button press event
			HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); // Toggle an LED connected to GPIOB Pin 1
			break;
	}
}

void ButtonDebounce(void *argument)
{
  /* USER CODE BEGIN ButtonDebounce */

  /* USER CODE END ButtonDebounce */
}

void StartDefaultTask(void *argument)
{
	/* USER CODE BEGIN defaultTask */
	/* Infinite loop */
	for(;;)
	{
		// HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
		osDelay(500);
	}
	/* USER CODE END defaultTask */
}