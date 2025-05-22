/**
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5StickCPlus2: https://github.com/m5stack/M5StickCPlus2
 */

#include <Arduino.h>
#include "M5StickCPlus2.h"

// Function to convert voltage to percentage
int voltageToPercentage(int voltage)
{
    // Assuming 3.7V is 0% and 4.2V is 100%
    int percentage = map(voltage, 3700, 4200, 0, 100);
    return constrain(percentage, 0, 100);
}

int getStableBatteryPercentage() {
    const int numReadings = 10;
    int totalVoltage = 0;

    for (int i = 0; i < numReadings; i++) {
        totalVoltage += StickCP2.Power.getBatteryVoltage();
        delay(10); // Small delay between readings
    }

    int averageVoltage = totalVoltage / numReadings;
    return voltageToPercentage(averageVoltage);
}

bool displayPaused = false;

void setup()
{   
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    Serial.begin(115200);
    Serial.println("M5StickCPlus2 initialized");
    delay(2000);

    StickCP2.Display.setBrightness(25);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
    StickCP2.Display.setTextSize(1);

    pinMode(32, INPUT_PULLUP); // Enable internal pull-down resistor for pin 32.
    pinMode(33, INPUT_PULLDOWN);
    pinMode(19, OUTPUT);    // Set pin 19 as an output.
}

void loop()
{
    StickCP2.update(); // Update button states

    // if (StickCP2.BtnB.wasClicked()) {
    //     StickCP2.Display.clear();
    //     // StickCP2.Speaker.tone(8000, 20);
    // }

    // if (StickCP2.BtnA.wasPressed()) {
    //     displayPaused = !displayPaused; // Toggle display update state
    // }

    if (!displayPaused) {
        StickCP2.Display.clear();
        int percentage = getStableBatteryPercentage();
        StickCP2.Display.setCursor(10, 30);
        StickCP2.Display.printf("BAT: %d%%\n", percentage);
        StickCP2.Display.setCursor(10, 50);
        StickCP2.Display.setTextSize(0.7);
        StickCP2.Display.printf("%dmV\n", StickCP2.Power.getBatteryVoltage());
        StickCP2.Display.setTextSize(1);

        int analog = analogRead(33);
        int digital = digitalRead(32);

        if (analog < 1500 && digital == 1) // No sensor attached
        {
            StickCP2.Display.setCursor(10, 120);
            StickCP2.Display.printf("No sensor\n");
        }
        else
        {       
            digitalWrite(19, digital);

            StickCP2.Display.setCursor(10, 90);
            StickCP2.Display.printf("Analog:%d\n", analog);

            if (digital == 1)
            {
                StickCP2.Display.setCursor(10, 120);
                StickCP2.Display.printf("NEEDS WATER\n");
            }
            else
            {
                StickCP2.Display.setCursor(10, 120);
                StickCP2.Display.printf("All good\n");
            }
        }
    }

    delay(1000);
}