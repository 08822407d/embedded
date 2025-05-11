#include <Arduino.h>
#include <Wire.h>
#include <M5Unified.h>
// #include <M5StickCPlus2.h>

#include "M5Unit_ScrollEncoder.hpp"
#include "M5Unit_JoyStick2.hpp"


M5UnitScroll scroll;
M5UnitJoystick2 joystick2;

extern void initBoard();

void setup() {
	delay(1000);

	initBoard();

	Wire.begin(38, 39, 400000UL);
	scroll.begin(&Wire, UNIT_SCROLL_ADDR, 38, 39, 400000UL);
	// joystick2.begin(&Wire1, UNIT_JOYSTICK2_ADDR, 19, 22);

	delay(500);

	Serial.printf("firmware version:%d, i2c address:0x%X\n", scroll.getFirmwareVersion(), scroll.getI2CAddress());
}

signed short int last_value = 0;

void loop() {
	// int16_t encoder_value = scroll.getEncoderValue();
	// Serial.println(encoder_value);


	// uint8_t bootloader_ver, firmware_ver;
	// uint16_t adc_x, adc_y;
	// bootloader_ver = joystick2.get_bootloader_version();
	// firmware_ver   = joystick2.get_firmware_version();
	// joystick2.get_joy_adc_16bits_value_xy(&adc_x, &adc_y);
	// Serial.printf("bootloader ver:%d, firmware ver:%d, x adc:%d, y adc:%d\n", bootloader_ver, firmware_ver, adc_x,
	// 		adc_y);


	delay(100);
}


void initBoard() {
	auto cfg = M5.config();

	cfg.serial_baudrate = 115200;   // default=115200. if "Serial" is not needed, set it to 0.

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