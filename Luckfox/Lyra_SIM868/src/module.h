#ifndef _MODULE_H_
#define _MODULE_H_


#define SIM868_PWR_PIN	3
#define SIM868_WAKE_PIN	58


	/* module.c */
	extern void genericInit(void);
	extern void genericNetInit(void);

	/* module_init.c */
	extern void check_start(void);
	extern void set_network(void);
	extern void check_network(void);

#endif /* _MODULE_H_ */