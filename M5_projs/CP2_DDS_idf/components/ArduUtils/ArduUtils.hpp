#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"


#define delay(ms)		vTaskDelay(pdMS_TO_TICKS(ms))
#define delayUntil(ms)	vTaskDelayUntil(pdMS_TO_TICKS(ms))