#include "timer.hpp"
#include "dac.hpp"


// PeriodicTimer::PeriodicTimer(esp_timer_cb_t callback, const char *name = "periodic") {
// 	timer_create_args.callback = callback;
// 	/* name is optional, but may help identify the timer when debugging */
// 	// timer_create_args.name = name;

// 	ESP_ERROR_CHECK(esp_timer_create(&timer_create_args, &timer_handler));
// }

// void PeriodicTimer::Start(void *arg) {
// 	period_ms = (uint64_t) arg;
// 	ESP_ERROR_CHECK(esp_timer_start_periodic(timer_handler, period_ms));
// }

// void PeriodicTimer::Stop() {
// 	ESP_ERROR_CHECK(esp_timer_stop(timer_handler));
// }


// PeriodicTimer *WriteDacTimer;




const esp_timer_create_args_t periodic_timer_args = {
	.callback = &sendToDac,
	.arg = (void*) &Wave,
	/* name is optional, but may help identify the timer when debugging */
	.name = "periodic"
};

esp_timer_handle_t periodic_timer;

void init_timer() {
	// ESP_ERROR_CHECK(esp_timer_init());
	ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
}

void start_timer(uint64_t period_ms) {
	ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, period_ms));
}

void stop_timer() {
	ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
}