#pragma once

#define MODULE_LOG_HEAD(module)					\
		do {									\
			Serial.printf("[ " #module " ]");	\
		} while (0)

#define MODULE_LOG_HEAD_INFO(module, fmt, ...)	\
		do {									\
			Serial.printf("[ " #module " ]"		\
				fmt, ##__VA_ARGS__);			\
		} while (0)

#define MODULE_LOG_TAIL(fmt, ...)				\
		do {									\
			Serial.printf(fmt, ##__VA_ARGS__);	\
		} while (0)

