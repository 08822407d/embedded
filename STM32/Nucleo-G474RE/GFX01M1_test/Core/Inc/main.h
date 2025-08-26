/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

#include "lcd_io.h"
#include "key_io.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define B1_EXTI_IRQn EXTI15_10_IRQn
#define RCC_OSC_IN_Pin GPIO_PIN_0
#define RCC_OSC_IN_GPIO_Port GPIOF
#define DISP_TE_Pin GPIO_PIN_0
#define DISP_TE_GPIO_Port GPIOA
#define DISP_TE_EXTI_IRQn EXTI0_IRQn
#define DISP_NRESET_Pin GPIO_PIN_1
#define DISP_NRESET_GPIO_Port GPIOA
#define SPIA_SCLK_Pin GPIO_PIN_5
#define SPIA_SCLK_GPIO_Port GPIOA
#define SPIA_MISO_Pin GPIO_PIN_6
#define SPIA_MISO_GPIO_Port GPIOA
#define SPIA_MOSI_Pin GPIO_PIN_7
#define SPIA_MOSI_GPIO_Port GPIOA
#define JOY_CENTER_Pin GPIO_PIN_8
#define JOY_CENTER_GPIO_Port GPIOC
#define JOY_LEFT_Pin GPIO_PIN_9
#define JOY_LEFT_GPIO_Port GPIOC
#define T_SWDIO_Pin GPIO_PIN_13
#define T_SWDIO_GPIO_Port GPIOA
#define T_SWCLK_Pin GPIO_PIN_14
#define T_SWCLK_GPIO_Port GPIOA
#define JOY_DOWN_Pin GPIO_PIN_10
#define JOY_DOWN_GPIO_Port GPIOC
#define JOY_RIGHT_Pin GPIO_PIN_11
#define JOY_RIGHT_GPIO_Port GPIOC
#define JOY_UP_Pin GPIO_PIN_12
#define JOY_UP_GPIO_Port GPIOC
#define SPIA_DCX_Pin GPIO_PIN_3
#define SPIA_DCX_GPIO_Port GPIOB
#define SPIA_NCS_Pin GPIO_PIN_5
#define SPIA_NCS_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
