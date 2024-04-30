/**
******************************************************************************
* @file    scripts.h
* @author  Waveshare Team
* @version V1.0.0
* @date    23-January-2018
* @brief   This file describes the sending of JavaScript codes to a client
*
******************************************************************************
*/
#include<SPIFFS.h>

#include "webpage.h"


void sendJS_A(WiFiClient client, IPAddress myIP)
{
	String ip_addr = "var self_IP = \'" + myIP.toString() + "\';\n";
	File JSfile_A = SPIFFS.open("/page_A.js", FILE_READ);
	if (JSfile_A)
		client.println(ip_addr + JSfile_A.readString());
	JSfile_A.close();
}

void sendJS_B(WiFiClient client)
{
	File JSfile_B = SPIFFS.open("/page_B.js", FILE_READ);
	if (JSfile_B)
		client.println(JSfile_B.readString());
	JSfile_B.close();
}

void sendJS_C(WiFiClient client)
{
	File JSfile_C = SPIFFS.open("/page_C.js", FILE_READ);
	if (JSfile_C)
		client.println(JSfile_C.readString());
	JSfile_C.close();
}

void sendJS_D(WiFiClient client)
{
	File JSfile_D = SPIFFS.open("/page_D.js", FILE_READ);
	if (JSfile_D)
		client.println(JSfile_D.readString());
	JSfile_D.close();
}