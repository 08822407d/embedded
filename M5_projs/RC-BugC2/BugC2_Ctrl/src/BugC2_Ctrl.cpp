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

#include "NetCfg.h"


typedef struct MotorSpeeds {
	int8_t FrontLeft;
	int8_t FrontRight;
	int8_t BackLeft;
	int8_t BackRight;
} MotorSpeeds_s ;


const char	*ssid		= AP_SSID;
const char	*password	= AP_PASSWD;
WiFiServer	server(WIFI_SERVER_PORT);
WiFiUDP		Udp_BugC2_Controller;

M5HatBugC BugC2;

int width = 0;
int height = 0;

extern void printUdpData(int udplength);
extern void setBugC2_Speed(int udplength);
extern MotorSpeeds_s calcAngleLength_to_MotorSpeeds(float leftAngle,
	float leftLength, float rightAngle, float rightLength);
extern void imuTest(void);
extern void motorTest(void);

void setup(void) {
	StickCP2.begin();
	StickCP2.Imu.init();
	Serial.begin(SERIAL_BAUD_RATE);
	BugC2.begin();
	BugC2.setAllMotorSpeed(0, 0, 0, 0);

	// StickCP2.Display.setRotation(1);
	width = StickCP2.Display.width();
	height = StickCP2.Display.height();
	
	StickCP2.Display.setTextSize(2);
	// Wire.begin(0, 26, 100000UL);


	// Set device in STA mode to begin with
	WiFi.softAPConfig(SERVER_IP, GATWAY_ADDR, SUBNET_ADDR);
	WiFi.softAP(ssid, password);
	server.begin();
	Udp_BugC2_Controller.begin(UDP_SERVER_PORT);

	IPAddress myIP = WiFi.softAPIP();
	StickCP2.Display.setCursor(0, 4);
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
			
			case 16:
				setBugC2_Speed(udplength);
				break;
			
			default:
				break;
		}
	} else {
		delay(50);
	}

	// motorTest();
	// imuTest();
}

void printUdpData(int udplength) {
	StickCP2.Display.fillRect(0, 44, width, height - 44, BLACK);

	IPAddress udp_client = Udp_BugC2_Controller.remoteIP();
	// Serial.println("Recieved Data.");
	StickCP2.Display.setCursor(0, 44);
	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.printf("From IP:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("%s\n", udp_client.toString());

	char udpdata[udplength];
	Udp_BugC2_Controller.read(udpdata, udplength);

	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.setCursor(0, 84);
	StickCP2.Display.printf("Recieved:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("0x%08x  ", *(uint64_t *)udpdata);
}

void setBugC2_Speed(int udplength) {
	StickCP2.Display.fillRect(0, 44, width, height - 48, BLACK);

	IPAddress udp_client = Udp_BugC2_Controller.remoteIP();
	// Serial.println("Recieved Data.");
	StickCP2.Display.setCursor(0, 44);
	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.printf("From IP:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("%s\n", udp_client.toString());

	float udpdata[udplength / sizeof(float)];
	Udp_BugC2_Controller.read((uint8_t *)udpdata, udplength);
	udpdata[0] = radians(udpdata[0]);
	udpdata[2] = radians(udpdata[2]);

	StickCP2.Display.setTextColor(GREEN);
	StickCP2.Display.setCursor(0, 84);
	StickCP2.Display.printf("Recieved:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("L-A: %.1f\n", *(float *)(udpdata));
	StickCP2.Display.printf("L-D: %.1f\n", *(float *)(udpdata + 1));
	StickCP2.Display.printf("R-A: %.1f\n", *(float *)(udpdata + 2));
	StickCP2.Display.printf("R-D: %.1f\n", *(float *)(udpdata + 3));

	MotorSpeeds_s speeds = calcAngleLength_to_MotorSpeeds(
			udpdata[0], udpdata[1], udpdata[2], udpdata[3]);

	BugC2.setAllMotorSpeed(speeds.FrontLeft,
		speeds.FrontRight, speeds.BackLeft, speeds.BackRight);
}

MotorSpeeds_s calcAngleLength_to_MotorSpeeds(float leftAngle,
		float leftLength, float rightAngle, float rightLength) {
	MotorSpeeds_s speeds;

	float speed = cos(leftAngle) * leftLength;
	float angle = sin(rightAngle) * rightLength;

	// speeds.FrontLeft = speed / 5;
	// speeds.FrontRight = -speed / 5;
	// speeds.BackLeft = speed / 5;
	// speeds.BackRight = -speed / 5;

	speeds.FrontLeft = speed /5 + angle / 10;
	speeds.FrontRight = -speed /5 + angle / 10;
	speeds.BackLeft = speed /5 + angle / 10;
	speeds.BackRight = -speed /5 + angle / 10;

	return speeds;
}




void imuTest() {
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

void motorTest() {
	BugC2.setAllMotorSpeed(10, 0, 0, 0);
	delay(500);
	BugC2.setAllMotorSpeed(0, 10, 0, 0);
	delay(500);
	BugC2.setAllMotorSpeed(0, 0, 10, 0);
	delay(500);
	BugC2.setAllMotorSpeed(0, 0, 0, 10);
	delay(500);

	BugC2.setAllMotorSpeed(0, 0, 0, 0);
	delay(3000);
}