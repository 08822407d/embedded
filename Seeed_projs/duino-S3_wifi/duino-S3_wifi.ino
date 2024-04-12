#include <WiFi.h>
#include <WiFiUdp.h>


#define	LOACAL_IP		IPAddress(192, 168, 4, 100 + SYSNUM)
#define	SERVER_IP		IPAddress(192, 168, 4, 1)
#define GATWAY_ADDR		IPAddress(192, 168, 4, 1)
// #define SUBNET_ADDR		IPAddress(255, 255, 255, 0)
#define SUBNET_ADDR		IPAddress(255, 255, 0, 0)
#define PRIMARY_DNS		IPAddress(8, 8, 8, 8)
#define SECONDARY_DNS	IPAddress(8, 8, 4, 4)
#define AP_SSID			"M5SticC-P2 AP"
#define AP_PASSWD		"77777777"

// #define EEPROM_SIZE 64
#define SERVER_PORT		1003
#define UDP_PACK_SIZE	8



const char *ssid		= AP_SSID;
const char *password	= AP_PASSWD;

uint64_t counter = 0;


WiFiUDP Udp;
uint32_t send_count  = 0;
uint8_t system_state = 0;
uint8_t SendBuff[UDP_PACK_SIZE];
void SendUDP(const uint8_t *Buffer) {
	if (WiFi.status() == WL_CONNECTED) {
		Udp.beginPacket(SERVER_IP, SERVER_PORT);
		Udp.write(Buffer, UDP_PACK_SIZE);
		Udp.endPacket();
	}
}


void setup() {
	// put your setup code here, to run once:
	Serial.begin(115200);
}


void loop() {

	delay(2000);
	Serial.printf("\nDisconnecting to %s", ssid);
	WiFi.disconnect(true);  // Disconnect wifi.  断开wifi连接
	WiFi.mode(WIFI_OFF);  // Set the wifi mode to off.  设置wifi模式为关闭
	Serial.println("\nDISCONNECTED!");
	delay(2000);


	Serial.printf("\nConnecting to %s", ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	Serial.println("\nCONNECTED!");

	Udp.begin(2000);

	counter++;
	*((uint64_t *)SendBuff) = counter;
	SendUDP(SendBuff);

	Udp.stop();
}