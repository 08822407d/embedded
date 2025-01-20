#ifndef _GPIO_H_
#define _GPIO_H_

#include "common_headers.h"


	enum GPIO_DIRECTION {
		GPIO_IN,
		GPIO_OUT,
	};
	typedef enum GPIO_DIRECTION GPIO_DIR_e;

	enum DIGITAL_LEVEL {
		DIGITAL_LOW,
		DIGITAL_HIGH,
	};
	typedef enum DIGITAL_LEVEL DIGI_LEV_e;

	int pinExport(uint16_t pin);
	int pinMode(uint16_t pin, GPIO_DIR_e dir);
	int pinWrite(uint16_t pin, DIGI_LEV_e level);
	#define digitalWrite pinWrite

#endif /* _GPIO_H_ */