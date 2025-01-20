#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "common_headers.h"


	int openSerial(int port);
	int configSerial(int serial_fd);

#endif /* _SERIAL_H_ */