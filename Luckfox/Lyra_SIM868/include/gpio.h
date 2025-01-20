#ifndef _GPIO_H_
#define _GPIO_H_

#include "common_headers.h"


#define GPIO_IN		0
#define GPIO_OUT	1


	int pinExport(int pin);
	int pinMode(int pin, int direction);

#endif /* _GPIO_H_ */