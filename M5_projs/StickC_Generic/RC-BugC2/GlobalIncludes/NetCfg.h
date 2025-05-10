#ifndef _RC_BUGC2_NETCFG_H_
#define _RC_BUGC2_NETCFG_H_

	#define SERIAL_BAUD_RATE	115200

	#define	SERVER_IP			IPAddress(10, 61, 4, 1)
	#define	CLIENT_IP			IPAddress(10, 61, 4, 103)
	#define GATWAY_ADDR			IPAddress(10, 61, 4, 1)
	#define SUBNET_ADDR			IPAddress(255, 255, 255, 0)
	#define PRIMARY_DNS			IPAddress(8, 8, 8, 8)
	#define SECONDARY_DNS		IPAddress(8, 8, 4, 4)
	#define AP_SSID				"M5StickC-P2 AP"
	#define AP_PASSWD			"77777777"

	#define WIFI_SERVER_PORT	80

	#define UDP_SERVER_PORT		1003
	#define UDP_CLIENT_PORT		2000

#endif /* _RC_BUGC2_NETCFG_H_ */