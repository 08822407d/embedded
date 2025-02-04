#ifndef _MODULE_H_
#define _MODULE_H_

#include "utils_defs.h"
#include "gpio.h"
#include "serial.h"


// #define SIM868_PWR_PIN	3
// #define SIM868_WAKE_PIN	58
#define SIM868_PWR_PIN	56
#define SIM868_WAKE_PIN	69

	extern int Serial0_fd;

	extern bool sendCMD_waitResp(int serial_fd, const char *str, const char *back);
	extern char* waitResp(const char *str, const char *back, int timeout);
	extern bool sendCMD_waitResp_AT(const char *str, const char *back, int timeout);
	extern void module_power(void);
	extern void module_wakeup(void);
	extern void genericInit(void);
	extern void genericNetInit(void);

	/* module_init.c */
	extern void check_start(int serial_fd);
	extern void set_network(int serial_fd);
	extern void check_network(int serial_fd);

#endif /* _MODULE_H_ */