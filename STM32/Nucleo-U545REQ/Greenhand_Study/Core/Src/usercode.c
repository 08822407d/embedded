#include "main.h"
#include "stm32u5xx_hal_gpio.h"
#include "stm32u5xx_it.h"
#include "app_freertos.h"
#include "tim.h"


void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
		case EXTI_Key1_Pin: // Assuming GPIO_PIN_0 is connected to a button
			// Handle button press event
			// HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); // Toggle an LED connected to GPIOB Pin 1
			break;
	}
}

int count = 0;
double percent= 0;
void HAL_GPIO_EXTI_Rising_Callback(uint16_t GPIO_Pin)
{
	switch (GPIO_Pin)
	{
		case USER_BUTTON_Pin: // Assuming GPIO_PIN_0 is connected to a button
			// Handle button press event
			// HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin); // Toggle an LED connected to GPIOB Pin 1
			/* 可选：运行时改占空比 */
			percent = (count++ * 20) % 100;
			uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim2);
			uint32_t ccr = (uint32_t) ((percent / 100.f) * (arr + 1) + 0.5f);
			__HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, ccr);
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

	HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

	/* Infinite loop */
	for(;;)
	{
		osDelay(500);
	}
	/* USER CODE END defaultTask */
}