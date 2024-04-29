#pragma once

#include <WiFi.h>

void sendCSS(WiFiClient client);
void sendHtml(WiFiClient client);

void sendJS_A(WiFiClient client, IPAddress myIP);
void sendJS_B(WiFiClient client);
void sendJS_C(WiFiClient client);
void sendJS_D(WiFiClient client);