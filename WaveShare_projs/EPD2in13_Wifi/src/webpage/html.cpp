/**
  ******************************************************************************
  * @file    html.h
  * @author  Waveshare Team
  * @version V1.0.0
  * @date    23-January-2018
  * @brief   This file describes the sending of HTML-page to a client
  *
  ******************************************************************************
  */
#include<SPIFFS.h>

#include "webpage.h"


void sendHtml(WiFiClient client)
{
	File HTMLfile = SPIFFS.open("/page.html", FILE_READ);
	if (HTMLfile)
		client.println(HTMLfile.readString());
	HTMLfile.close();
}