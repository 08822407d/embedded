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

#define	LOACAL_IP		IPAddress(192,168,4,1)
#define GATWAY_ADDR		IPAddress(192, 168, 4, 1)
#define SUBNET_ADDR		IPAddress(255, 255, 255, 0)
#define AP_SSID			"M5SticC-P2 AP"
#define AP_PASSWD		"77777777"

const char	*ssid		= AP_SSID;
const char	*password	= AP_PASSWD;
WiFiServer	server(80);
WiFiUDP		Udp_BugC2_Controller;

int width = 0;
int height = 0;

void setup(void) {
	StickCP2.begin();
	Serial.begin(115200);

	width = StickCP2.Display.width();
	height = StickCP2.Display.height();
	StickCP2.Display.setRotation(1);
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
	StickCP2.Display.printf("This AP IP:\n");
	StickCP2.Display.setTextColor(ORANGE);
	StickCP2.Display.printf("  %s", myIP.toString());
}


void loop(void) {
	int udplength = Udp_BugC2_Controller.parsePacket();
	if (udplength == 8) {
		StickCP2.Display.fillRect(0, 48, 240, 175 - 48, BLACK);
		delay(50);

		IPAddress udp_client = Udp_BugC2_Controller.remoteIP();
		// Serial.println("Recieved Data.");
		StickCP2.Display.setCursor(0, 48);
		StickCP2.Display.setTextColor(GREEN);
		StickCP2.Display.printf("From client: \n");
		StickCP2.Display.setTextColor(ORANGE);
		StickCP2.Display.printf("  %s\n", udp_client.toString());

		char udpdata[udplength];
		Udp_BugC2_Controller.read(udpdata, udplength);

		StickCP2.Display.setTextColor(GREEN);
		StickCP2.Display.setCursor(0, 88);
		StickCP2.Display.printf("Recieved Data:\n");
		StickCP2.Display.setTextColor(ORANGE);
		StickCP2.Display.printf("  0x%08x  ", *(uint64_t *)udpdata);


		// if ((udpdata[0] == 0xAA) && (udpdata[1] == 0x55) &&
		// 	(udpdata[7] == 0xee)) {
		// 	for (int i = 0; i < 8; i++) {
		// 		Serial.printf("%02X ", udpdata[i]);
		// 	}
		// 	Serial.println();
		// 	if (udpdata[6] == 0x01) {
		// 		IIC_ReState = Setspeed(udpdata[3] - 100, udpdata[4] - 100,
		// 							   udpdata[5] - 100);
		// 	} else {
		// 		IIC_ReState = Setspeed(0, 0, 0);
		// 	}
		// } else {
		// 	IIC_ReState = Setspeed(0, 0, 0);
		// }
	}

	delay(200);
}
