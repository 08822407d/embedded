/*
 *  This sketch demonstrates how to scan WiFi networks.
 *  The API is almost the same as with the WiFi Shield library,
 *  the most obvious difference being the different file you need to include:
 */
#include <Arduino.h>


const int analogInPin = A0;
int sensorValue = 0;

void setup()
{
	Serial.begin(115200);
	Serial.println("Setup done");
}

void loop()
{
	// read the analog in value:
	sensorValue = analogRead(analogInPin);
	Serial.println(sensorValue);
	// Wait a bit before scanning again
	delay(10);
}