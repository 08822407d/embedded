#include "module.h"


void check_start(int serial_fd)
{
	if (sendCMD_waitResp(serial_fd, "AT", "OK")) {
		printf("------SIM7080G is ready------\n");
	} else {
		module_power();
		printf("------SIM7080G is starting up, please wait------\n");
		delay(5000);
	}
	while (1) {
		if (sendCMD_waitResp(serial_fd, "AT", "OK"))
			break;
		delay(500);
	}
}

void set_network(int serial_fd)
{
	// Serial.printf("Setting to NB-IoT mode:\n");
	// sendCMD_waitResp("AT+CFUN=0", "OK", 2000);
	// sendCMD_waitResp("AT+CNMP=38", "OK", 2000); // Select LTE mode
	// sendCMD_waitResp("AT+CMNB=2", "OK", 2000);  // Select NB-IoT mode,if Cat-M，please set to 1
	// sendCMD_waitResp("AT+CFUN=1", "OK", 2000);
	// DEV_Delay_ms(5000);
}

void check_network(int serial_fd)
{
	// char *get_resp_info, getapn1[20];
	// int j = 0, k = 0;
	// char CNCFG[100] = "AT+CNCFG=0,1,\"";
	// char a[] = "\"";
	// if (sendCMD_waitResp("AT+CPIN?", "READY", 2000) == 1)
	// 	Serial.printf("------Please check whether the sim card has been inserted!------\n");
	// for (int i = 1; i < 3; i++) {
	// 	if (sendCMD_waitResp("AT+CGATT?", "1", 2000) == 1) {
	// 		Serial.printf("------SIM7080G is online------\r\n");
	// 		break;
	// 	} else {
	// 		Serial.printf("------SIM7080G is offline, please wait...------\r\n");
	// 		DEV_Delay_ms(5000);
	// 		continue;
	// 	}
	// }
	// sendCMD_waitResp("AT+CSQ", "OK", 2000);
	// sendCMD_waitResp("AT+CPSI?", "OK", 2000);
	// sendCMD_waitResp("AT+COPS?", "OK", 2000);

	// get_resp_info = waitResp("AT+CGNAPN", "OK", 2000);
	// while (1) {
	// 	if (get_resp_info[j] == '\"') {
	// 		while (1) {
	// 			getapn1[k] = get_resp_info[j + 1];
	// 			DEV_Delay_ms(100);
	// 			j++;
	// 			k++;
	// 			if (get_resp_info[j + 1] == '\"')
	// 				break;
	// 		}
	// 		break;
	// 	}
	// 	j++;
	// }
	// Serial.printf("getapn1:%s\r\n", getapn1);
	// strcat(CNCFG, getapn1);
	// strcat(CNCFG, a);
	// sendCMD_waitResp(CNCFG, "OK", 2000);
	// sendCMD_waitResp("AT+CNACT=0,1", "OK", 2000);
	// sendCMD_waitResp_AT("AT+CNACT?", "OK", 2000);
}