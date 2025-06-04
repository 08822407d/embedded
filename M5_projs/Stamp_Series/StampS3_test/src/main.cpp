/*
 *******************************************************************************
 * Copyright (c) 2022 by M5Stack
 *                  Equipped with StampS3 sample source code
 *                          配套  StampS3 示例源代码
 * Visit for more information: https://docs.m5stack.com/en/core/stamps3
 * 获取更多资料请访问: https://docs.m5stack.com/zh_CN/core/stamps3
 *
 * Describe: LED Show example.  LED展示示例
 * Date: 2023/1/11
 *******************************************************************************
 Press button to change LED status
 按下按键切换LED状态.
 */
#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include <FastLED.h>


extern void initBoard();


#define PIN_BUTTON 0
#define PIN_LED    21
#define NUM_LEDS   1

CRGB leds[NUM_LEDS];
uint8_t led_ih             = 0;
uint8_t led_status         = 0;
String led_status_string[] = {"Rainbow", "Red", "Green", "Blue"};

/* After StampS3 is started or reset
   the program in the setUp () function will be run, and this part will only be
   run once.
   在StampS3启动或者复位后，即会开始执行setup()函数中的程序，该部分只会执行一次。
*/
void setup() {
	initBoard();
	Serial.begin(115200);
	Serial.println("StampS3 demo!");

	pinMode(PIN_BUTTON, INPUT);

	FastLED.addLeds<WS2812, PIN_LED, GRB>(leds, NUM_LEDS);
}

/* After the program in setup() runs, it runs the program in loop()
  The loop() function is an infinite loop in which the program runs repeatedly
  在setup()函数中的程序执行完后，会接着执行loop()函数中的程序
  loop()函数是一个死循环，其中的程序会不断的重复运行
*/
void loop() {
	switch (led_status) {
		case 0:
			leds[0] = CHSV(led_ih, 255, 255);
			break;
		case 1:
			leds[0] = CRGB::Red;
			break;
		case 2:
			leds[0] = CRGB::Green;
			break;
		case 3:
			leds[0] = CRGB::Blue;
			break;
		default:
			break;
	}
	FastLED.show();
	led_ih++;
	delay(15);

	if (!digitalRead(PIN_BUTTON)) {
		delay(5);
		if (!digitalRead(PIN_BUTTON)) {
			led_status++;
			if (led_status > 3) led_status = 0;
			while (!digitalRead(PIN_BUTTON))
				;
			Serial.print("LED status updated: ");
			Serial.println(led_status_string[led_status]);
		}
	}
}


void initBoard() {
	auto cfg = M5.config();

#if defined ( ARDUINO )
	cfg.serial_baudrate = 115200;   // default=115200. if "Serial" is not needed, set it to 0.
#endif
	cfg.clear_display = true;  // default=true. clear the screen when begin.
	cfg.output_power  = true;  // default=true. use external port 5V output.
	cfg.internal_imu  = true;  // default=true. use internal IMU.
	cfg.internal_rtc  = true;  // default=true. use internal RTC.
	cfg.internal_spk  = true;  // default=true. use internal speaker.
	cfg.internal_mic  = true;  // default=true. use internal microphone.
	cfg.external_imu  = true;  // default=false. use Unit Accel & Gyro.
	cfg.external_rtc  = true;  // default=false. use Unit RTC.
	cfg.led_brightness = 64;   // default= 0. system LED brightness (0=off / 255=max) (※ not NeoPixel)

	// external speaker setting.
	cfg.external_speaker.module_display = true;  // default=false. use ModuleDisplay AudioOutput
	cfg.external_speaker.hat_spk        = true;  // default=false. use HAT SPK
	cfg.external_speaker.hat_spk2       = true;  // default=false. use HAT SPK2
	cfg.external_speaker.atomic_spk     = true;  // default=false. use ATOMIC SPK
	cfg.external_speaker.atomic_echo    = true;  // default=false. use ATOMIC ECHO BASE
	cfg.external_speaker.module_rca     = false; // default=false. use ModuleRCA AudioOutput

	// external display setting. (Pre-include required)
	cfg.external_display.module_display = true;  // default=true. use ModuleDisplay
	cfg.external_display.atom_display   = true;  // default=true. use AtomDisplay
	cfg.external_display.unit_glass     = false; // default=true. use UnitGLASS
	cfg.external_display.unit_glass2    = false; // default=true. use UnitGLASS2
	cfg.external_display.unit_oled      = false; // default=true. use UnitOLED
	cfg.external_display.unit_mini_oled = false; // default=true. use UnitMiniOLED
	cfg.external_display.unit_lcd       = false; // default=true. use UnitLCD
	cfg.external_display.unit_rca       = false; // default=true. use UnitRCA VideoOutput
	cfg.external_display.module_rca     = false; // default=true. use ModuleRCA VideoOutput
/*
※ Unit OLED, Unit Mini OLED, Unit GLASS2 cannot be distinguished at runtime and may be misidentified as each other.

※ Display with auto-detection
 - module_display
 - atom_display
 - unit_glass
 - unit_glass2
 - unit_oled
 - unit_mini_oled
 - unit_lcd

※ Displays that cannot be auto-detected
 - module_rca
 - unit_rca

※ Note that if you enable a display that cannot be auto-detected, 
   it will operate as if it were connected, even if it is not actually connected.
   When RCA is enabled, it consumes a lot of memory to allocate the frame buffer.
//*/

// Set individual parameters for external displays.
// (※ Use only the items you wish to change. Basically, it can be omitted.)
#if defined ( __M5GFX_M5ATOMDISPLAY__ ) // setting for ATOM Display.
	// cfg.atom_display.logical_width  = 1280;
	// cfg.atom_display.logical_height = 720;
	// cfg.atom_display.refresh_rate   = 60;
#endif
#if defined ( __M5GFX_M5MODULEDISPLAY__ ) // setting for Module Display.
	// cfg.module_display.logical_width  = 1280;
	// cfg.module_display.logical_height = 720;
	// cfg.module_display.refresh_rate   = 60;
#endif
#if defined ( __M5GFX_M5MODULERCA__ ) // setting for Module RCA.
	// cfg.module_rca.logical_width  = 216;
	// cfg.module_rca.logical_height = 144;
	// cfg.module_rca.signal_type    = M5ModuleRCA::signal_type_t::PAL;
	// cfg.module_rca.use_psram      = M5ModuleRCA::use_psram_t::psram_use;
#endif
#if defined ( __M5GFX_M5UNITRCA__ ) // setting for Unit RCA.
	// cfg.unit_rca.logical_width  = 216;
	// cfg.unit_rca.logical_height = 144;
	// cfg.unit_rca.signal_type    = M5UnitRCA::signal_type_t::PAL;
	// cfg.unit_rca.use_psram      = M5UnitRCA::use_psram_t::psram_use;
#endif
#if defined ( __M5GFX_M5UNITGLASS__ ) // setting for Unit GLASS.
	// cfg.unit_glass.pin_sda  = GPIO_NUM_21;
	// cfg.unit_glass.pin_scl  = GPIO_NUM_22;
	// cfg.unit_glass.i2c_addr = 0x3D;
	// cfg.unit_glass.i2c_freq = 400000;
	// cfg.unit_glass.i2c_port = I2C_NUM_0;
#endif
#if defined ( __M5GFX_M5UNITGLASS2__ ) // setting for Unit GLASS2.
	// cfg.unit_glass2.pin_sda  = GPIO_NUM_21;
	// cfg.unit_glass2.pin_scl  = GPIO_NUM_22;
	// cfg.unit_glass2.i2c_addr = 0x3C;
	// cfg.unit_glass2.i2c_freq = 400000;
	// cfg.unit_glass2.i2c_port = I2C_NUM_0;
#endif
#if defined ( __M5GFX_M5UNITOLED__ ) // setting for Unit OLED.
	// cfg.unit_oled.pin_sda  = GPIO_NUM_21;
	// cfg.unit_oled.pin_scl  = GPIO_NUM_22;
	// cfg.unit_oled.i2c_addr = 0x3C;
	// cfg.unit_oled.i2c_freq = 400000;
	// cfg.unit_oled.i2c_port = I2C_NUM_0;
#endif
#if defined ( __M5GFX_M5UNITMINIOLED__ ) // setting for Unit Mini OLED.
	// cfg.unit_mini_oled.pin_sda  = GPIO_NUM_21;
	// cfg.unit_mini_oled.pin_scl  = GPIO_NUM_22;
	// cfg.unit_mini_oled.i2c_addr = 0x3C;
	// cfg.unit_mini_oled.i2c_freq = 400000;
	// cfg.unit_mini_oled.i2c_port = I2C_NUM_0;
#endif
#if defined ( __M5GFX_M5UNITLCD__ ) // setting for Unit LCD.
	// cfg.unit_lcd.pin_sda  = GPIO_NUM_21;
	// cfg.unit_lcd.pin_scl  = GPIO_NUM_22;
	// cfg.unit_lcd.i2c_addr = 0x3E;
	// cfg.unit_lcd.i2c_freq = 400000;
	// cfg.unit_lcd.i2c_port = I2C_NUM_0;
#endif

	// begin M5Unified.
	M5.begin(cfg);
}