#ifndef _SERIAL_H_
#define _SERIAL_H_

#include "common_headers.h"


	extern int openSerial(int port);
	extern int configSerial(int serial_fd);
	extern int readSerial(int serial_fd, char *rx_buffer, int bufflen);
	extern int writeSerial(int serial_fd, char *tx_buffer, int bufflen);

#endif /* _SERIAL_H_ */