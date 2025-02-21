#include "DEV_Config.hpp"

#if (USING_SIM_MODULE == MODULE_SIM868)

	void check_start()
	{
		while(1){
			sendCMD_MustResp("ATE1", "OK", 2000);
			DEV_Delay_ms(2000);
			if(sendCMD_MustResp("AT", "OK", 2000) == 1){
				printf("SIM868 is ready\r\n");
				break;
			}
			else{
				module_power_on();
				printf("SIM868 is starting up, please wait...\r\n");
				DEV_Delay_ms(2000);
			}
		}
	}

	/**Check the network status**/

	void check_network()
	{
		char CSTT[] = "AT+CSTT=\"CMNET\"" ;
		for(int i=1;i<3;i++){
			if(sendCMD_MustResp("AT+CGREG?", "0,1", 2000) == 1){
				printf("SIM868 is online\r\n");
				break;
			}
			else{
				printf("SIM868 is offline, please wait...\r\n");
				DEV_Delay_ms(2000);
				continue;
			}
		}
		sendCMD_MustResp("AT+CPIN?", "OK", 2000);
		sendCMD_MustResp("AT+CSQ", "OK", 2000);
		sendCMD_MustResp("AT+COPS?", "OK", 2000);
		sendCMD_MustResp("AT+CGATT?", "OK", 2000);
		sendCMD_MustResp("AT+CGDCONT?", "OK", 2000);
		sendCMD_MustResp("AT+CSTT?", "OK", 2000);
		sendCMD_MustResp(CSTT, "OK", 2000);
		sendCMD_MustResp("AT+CIICR", "OK", 2000);
		sendCMD_MustResp("AT+CIFSR", "OK", 2000);
	}

#endif