/**
 * @file rtc.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief
 * @version 0.1
 * @date 2023-12-12
 *
 *
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5StickCPlus2: https://github.com/m5stack/M5StickCPlus2
 */
#include <M5StickCPlus2.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "M5HatBugC2.h"


#define	LOACAL_IP		IPAddress(192,168,4,1)
#define GATWAY_ADDR		IPAddress(192, 168, 4, 1)
#define SUBNET_ADDR		IPAddress(255, 255, 255, 0)
#define AP_SSID			"M5SticC-P2 AP"
#define AP_PASSWD		"77777777"

const char	*ssid		= AP_SSID;
const char	*password	= AP_PASSWD;
WiFiServer	server(80);
WiFiUDP		Udp_BugC2_Controller;

M5HatBugC BugC2;

int width = 0;
int height = 0;

extern void printUdpData(int udplength);
extern void setBugC2_Speed(int udplength);
extern void imuTest(void);

void setup(void) {
	StickCP2.begin();
	StickCP2.Imu.init();
	Serial.begin(115200);
	BugC2.begin();

	// StickCP2.Display.setRotation(1);
	width = StickCP2.Display.width();
	height = StickCP2.Display.height();
	
	StickCP2.Display.setTextSize(2);
	// Wire.begin(0, 26, 100000UL);

	// uint64_t chipid	= ESP.getEfuseMac();
	// ssid		= (ssid + String((uint32_t)(chipid >> 32), HEX)).c_str();

	// Set device in STA mode to begin with
	WiFi.softAPConfig(LOACAL_IP, GATWAY_ADDR, SUBNET_ADDR);
	WiFi.softAP(ssid, password);
	server.begin();
	Udp_BugC2_Controller.begin(1003);

	IPAddress myIP = WiFi.softAPIP();
	StickCP2.Display.setCursor(0, 8);
	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.printf("SoftAP:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("%s", myIP.toString());
}


void loop(void) {
	int udplength = Udp_BugC2_Controller.parsePacket();
	if (udplength) {
		switch (udplength) {
			case 8:
				printUdpData(udplength);
				break;
			
			case 64:
				setBugC2_Speed(udplength);
				break;
			
			default:
				break;
		}
	} else {
		delay(50);
	}

	// imuTest();
}

void printUdpData(int udplength)
{
	StickCP2.Display.fillRect(0, 48, width, height - 48, BLACK);

	IPAddress udp_client = Udp_BugC2_Controller.remoteIP();
	// Serial.println("Recieved Data.");
	StickCP2.Display.setCursor(0, 48);
	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.printf("From IP:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("%s\n", udp_client.toString());

	char udpdata[udplength];
	Udp_BugC2_Controller.read(udpdata, udplength);

	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.setCursor(0, 88);
	StickCP2.Display.printf("Recieved:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("0x%08x  ", *(uint64_t *)udpdata);
}

void setBugC2_Speed(int udplength)
{
	StickCP2.Display.fillRect(0, 48, width, height - 48, BLACK);

	IPAddress udp_client = Udp_BugC2_Controller.remoteIP();
	// Serial.println("Recieved Data.");
	StickCP2.Display.setCursor(0, 48);
	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.printf("From IP:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("%s\n", udp_client.toString());

	char udpdata[udplength];
	Udp_BugC2_Controller.read(udpdata, udplength);

	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.setCursor(0, 88);
	StickCP2.Display.printf("Recieved:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("Angle: %.1f  ", *(float *)(udpdata));
	StickCP2.Display.printf("Speed: %d  ", *(float *)(udpdata + 1));
}


void imuTest()
{
	float accX = 0;
	float accY = 0;
	float accZ = 0;
	float gyroX = 0;
	float gyroY = 0;
	float gyroZ = 0;
	float magX = 0;
	float magY = 0;
	float magZ = 0;
	StickCP2.Imu.getAccel(&accX, &accY, &accZ);
	StickCP2.Imu.getGyro(&gyroX, &gyroY, &gyroZ);
	StickCP2.Imu.getMag(&magX, &magY, &magZ);


	StickCP2.Display.fillRect(0, 0, width, height, BLACK);
	StickCP2.Display.setCursor(4, 4);

	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.println("Pose:\n");
	StickCP2.Display.setCursor(4, 24);
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("X:\n  %.2f\nY:\n  %.2f\nZ:\n  %.2f\n", magX, magY, magZ);

	delay(100);
}