#include "LoraSx1262.h"

char *payload = "Hello world.  This a pretty long payload. We can transmit up to 255 bytes at once, which is pretty neat if you ask me";
LoraSx1262* radio;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Booted");

  radio = new LoraSx1262();
}

void loop() {
  Serial.print("Transmitting... ");
  radio->transmit((byte *)payload,strlen(payload));
  Serial.println("Done!");

  delay(1000);
}