#include <M5StickCPlus.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "NetCfg.h"

#include "JoyC.h"



int width = 0;
int height = 0;
const char *ssid		= AP_SSID;
const char *password	= AP_PASSWD;
WiFiUDP Udp;
JoyC joyc;


extern void sendAngleLength(void);

extern void sendTest(void);
extern void imuTest(void);


void SendUDP(const uint8_t *Buffer, size_t packlen) {
	if (WiFi.status() == WL_CONNECTED) {
		M5.Lcd.printf("\n");
		M5.Lcd.setCursor(M5.Lcd.getCursorX(),
			M5.Lcd.getCursorY() + 4);

		M5.Lcd.setTextColor(GREEN);
		M5.Lcd.printf("Pack Size:\n");
		M5.Lcd.setTextColor(ORANGE);
		M5.Lcd.printf(" %d\n", packlen);

		Udp.beginPacket(SERVER_IP, UDP_SERVER_PORT);
		Udp.write(Buffer, packlen);
		Udp.endPacket();
	}
}


int16_t curX = 0;
int16_t curY = 0;
char progress_char[] = "|/-\\"; 
void setup() {
	M5.begin();
	// M5.Imu.Init();
	Serial.begin(SERIAL_BAUD_RATE);

	// M5.Lcd.setRotation(1);
	width = M5.Lcd.width();
	height = M5.Lcd.height();

	M5.Lcd.setTextSize(2);
	M5.Lcd.setCursor(4, 4);
	// put your setup code here, to run once:

	if (!WiFi.config(CLIENT_IP, GATWAY_ADDR, SUBNET_ADDR, PRIMARY_DNS, SECONDARY_DNS)) {
		M5.Lcd.println("STA Failed to configure\n");
	}
	M5.Lcd.setTextColor(GREEN);
	Serial.printf("Connecting to %s", ssid);
	M5.Lcd.printf("Connecting:\n");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %s", ssid);
	M5.Lcd.setTextColor(GREEN);

	int wait_count = 0;
	curX = M5.Lcd.getCursorX();
	curY = M5.Lcd.getCursorY();
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		Serial.print(".");
		M5.Lcd.printf(" %c ", progress_char[wait_count % 4]);
		M5.Lcd.setCursor(curX, curY);
		wait_count++;

		delay(250);

		M5.Lcd.fillRect(curX, curY, curX + 20, curY + 20, BLACK);
	}

	Serial.println("CONNECTED!\n");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.println("\nCONNECTED!\n");
	M5.Lcd.setTextColor(GREEN);

	Udp.begin(UDP_CLIENT_PORT);

	delay(1000);

	curX = M5.Lcd.getCursorX();
	curY = M5.Lcd.getCursorY();

	joyc.Init();
}



void loop() {
	sendAngleLength();

	// sendTest();
	// imuTest();
}

void sendAngleLength() {
	M5.update();

	float leftAngle = joyc.GetAngle(0) / 1.0;
	if (leftAngle > 360)
		leftAngle = 360;
	float leftDistance = joyc.GetDistance(0) / 1.0;
	if (leftDistance > 100)
		leftDistance = 100;

	float rightAngle = joyc.GetAngle(1) / 1.0;
	if (rightAngle > 360)
		rightAngle = 360;
	float rightDistance = joyc.GetDistance(1) / 1.0;
	if (rightDistance > 100)
		rightDistance = 100;


	M5.Lcd.fillRect(curX, curY, width - curX, height - curY, BLACK);
	M5.Lcd.setCursor(0, curY + 4);

	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("L-A:");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %3.2f\n", leftAngle);
	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("L-D:");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %3.2f\n", leftDistance);
	
	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("R-A:");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %3.2f\n", rightAngle);
	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("R-D:");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %3.2f\n", rightDistance);


	float SendBuff[4];
	SendBuff[0] = leftAngle;
	SendBuff[1] = leftDistance;
	SendBuff[2] = rightAngle;
	SendBuff[3] = rightDistance;
	SendUDP((uint8_t *)SendBuff, sizeof(SendBuff));

	delay(50);
}

void sendTest() {
	M5.Lcd.fillRect(0, 0, width, height, BLACK);
	M5.Lcd.setCursor(0, 4);

	if (!WiFi.config(CLIENT_IP, GATWAY_ADDR, SUBNET_ADDR, PRIMARY_DNS, SECONDARY_DNS)) {
		M5.Lcd.println("STA Failed to configure\n");
	}
	M5.Lcd.setTextColor(GREEN);
	Serial.printf("Connecting to %s", ssid);
	M5.Lcd.printf("Connecting to :\n");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf("  %s", ssid);
	M5.Lcd.setTextColor(GREEN);

	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(250);
		Serial.print(".");
		M5.Lcd.print(".");
	}

	Serial.println("CONNECTED!\n");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.println("CONNECTED!\n");
	M5.Lcd.setTextColor(GREEN);



	Udp.begin(UDP_CLIENT_PORT);

	size_t packelen = 8;
	uint8_t SendBuff[packelen];
	*(uint32_t *)(SendBuff + 1) = 0;

	SendBuff[0]++;
	SendUDP(SendBuff, sizeof(SendBuff));
	SendBuff[1]++;
	SendBuff[1]++;
	SendUDP(SendBuff, sizeof(SendBuff));

	Udp.stop();



	delay(250);
	Serial.printf("Disconnecting with %s\n", ssid);
	M5.Lcd.printf("Disconnecting with :\n");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf("  %s\n", ssid);
	M5.Lcd.setTextColor(GREEN);

	WiFi.disconnect(true);  // Disconnect wifi.  断开wifi连接
	WiFi.mode(WIFI_OFF);  // Set the wifi mode to off.  设置wifi模式为关闭

	Serial.println("DISCONNECTED!\n");
	M5.Lcd.println("DISCONNECTED!\n");
	delay(500);
}

void imuTest() {
	float accX = 0;
	float accY = 0;
	float accZ = 0;
	float gyroX = 0;
	float gyroY = 0;
	float gyroZ = 0;
	M5.Imu.getAccelData(&accX, &accY, &accZ);
	M5.Imu.getGyroData(&gyroX, &gyroY, &gyroZ);


	M5.Lcd.fillRect(0, 0, width, height, BLACK);
	M5.Lcd.setCursor(4, 4);

	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.println("Pose:\n");
	M5.Lcd.setCursor(4, 24);
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf("accX:\n  %.2f\naccY:\n  %.2f\naccZ:\n  %.2f\n", gyroX, gyroY, gyroZ);

	delay(100);
}
