#include "DEV_Config.hpp"

// MQTT Server info
char mqtt_host[]	= "47.89.22.46,";           
char mqtt_port[]	= "1883";
char mqtt_msg[]		= "on";

void mqttTest()
{
	Serial.println("Starting MQTT test ...");

	char SMCONF[100] = "AT+SMCONF=\"URL\",";
	strcat(SMCONF,mqtt_host);
	strcat(SMCONF,mqtt_port);
	sendCMD_MustResp(SMCONF, "OK", 2000);
	sendCMD_MustResp("AT+SMCONF=\"KEEPTIME\",600", "OK", 2000);
	sendCMD_MustResp("AT+SMCONF=\"CLIENTID\",\"Pico_SIM7080G\"", "OK", 2000);
	sendCMD_MustResp("AT+SMCONN", "OK", 2000);
	sendCMD_MustResp("AT+SMSUB=\"mqtt\",1", "OK", 2000);
	sendCMD_MustResp("AT+SMPUB=\"mqtt\",2,1,0", "OK", 2000);
	Serial.println(mqtt_msg);
	DEV_Delay_ms(2000);
	sendCMD_MustResp("AT+SMUNSUB=\"mqtt\"", "OK", 2000);
	Serial.printf("send message successfully!\r\n");
	sendCMD_MustResp("AT+SMDISC", "OK", 2000);
}