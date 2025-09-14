/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
/* Definitions for GuiTask */
osThreadId_t GuiTaskHandle;
const osThreadAttr_t GuiTask_attributes = {
  .name = "GuiTask",
  .stack_size = 4096 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for UserTask */
osThreadId_t UserTaskHandle;
const osThreadAttr_t UserTask_attributes = {
  .name = "UserTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LvglTickTimer */
osTimerId_t LvglTickTimerHandle;
const osTimerAttr_t LvglTickTimer_attributes = {
  .name = "LvglTickTimer"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartGuiTask(void *argument);
void StartUserTask(void *argument);
void LvglTick_Callback(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of LvglTickTimer */
  LvglTickTimerHandle = osTimerNew(LvglTick_Callback, osTimerPeriodic, NULL, &LvglTickTimer_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of GuiTask */
  GuiTaskHandle = osThreadNew(StartGuiTask, NULL, &GuiTask_attributes);

  /* creation of UserTask */
  UserTaskHandle = osThreadNew(StartUserTask, NULL, &UserTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartGuiTask */
/**
  * @brief  Function implementing the GuiTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartGuiTask */
__weak void StartGuiTask(void *argument)
{
  /* USER CODE BEGIN StartGuiTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartGuiTask */
}

/* USER CODE BEGIN Header_StartUserTask */
/**
* @brief Function implementing the UserTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartUserTask */
__weak void StartUserTask(void *argument)
{
  /* USER CODE BEGIN StartUserTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartUserTask */
}

/* LvglTick_Callback function */
__weak void LvglTick_Callback(void *argument)
{
  /* USER CODE BEGIN LvglTick_Callback */

  /* USER CODE END LvglTick_Callback */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

