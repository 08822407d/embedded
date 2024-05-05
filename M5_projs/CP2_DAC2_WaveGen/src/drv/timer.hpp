#pragma once

#include <esp_timer.h>


// class PeriodicTimer {
// public:
// 	uint64_t    period_ms;

// 	PeriodicTimer(esp_timer_cb_t callback, const char *name);
// 	void Start(void *arg);
// 	void Stop();

// private:
// 	esp_timer_create_args_t timer_create_args;
// 	esp_timer_handle_t      timer_handler;
// };


// extern PeriodicTimer *WriteDacTimer;

void init_timer(void);
void start_timer(uint64_t period_ms);
void stop_timer(void);