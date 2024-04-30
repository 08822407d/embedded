/**
	******************************************************************************
	* @file    css.h
	* @author  Waveshare Team
	* @version V1.0.0
	* @date    23-January-2018
	* @brief   This file describes the sending of Cascade Style Sheet to a client
	*
	******************************************************************************
	*/

#include <SPIFFS.h>

#include "webpage.h"


void sendCSS(WiFiClient client) {
	File css_file = SPIFFS.open("/page.css", FILE_READ);
	if (css_file)
		client.println(css_file.readString());
	css_file.close();
}