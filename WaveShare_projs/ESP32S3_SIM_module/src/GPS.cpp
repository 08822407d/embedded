#include "DEV_Config.hpp"

void GPSTest()
{
	// int count = 0;
	// Serial.printf("Start GPS session...\r\n");
	// sendCMD_MustResp("AT+CGNSPWR=1", "OK", 2000);
	// DEV_Delay_ms(2000);
	// for (int i = 1; i < 10; i++) {
	// 	if (sendCMD_MustResp("AT+CGNSINF", ",,,,", 2000) == 1) {
	// 		Serial.printf("GPS is not ready\r\n");
	// 		if (i >= 9) {
	// 			Serial.printf("GPS positioning failed, please check the GPS antenna!\r\n");
	// 			sendCMD_MustResp("AT+CGNSPWR=0", "OK", 2000);
	// 		} else {
	// 			Serial.printf("wait...\r\n");
	// 			DEV_Delay_ms(2000);
	// 			continue;
	// 		}
	// 	} else {
	// 		if (count <= 3) {
	// 			count++;
	// 			Serial.printf("GPS info:\r\n");
	// 		} else {
	// 			sendCMD_MustResp("AT+CGNSPWR=0", "OK", 2000);
	// 			break;
	// 		}
	// 	}
	// }
}
