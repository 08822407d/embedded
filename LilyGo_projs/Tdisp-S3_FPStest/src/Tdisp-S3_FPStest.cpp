#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>



#define PIN_POWER_ON		15
#define SCREEN_ROTAION		3
#define AA_FONT_SMALL		"NotoSansBold15"
#define AA_FONT_LARGE		"NotoSansBold36"
#define AA_FONT_MONO		"NotoSansMonoSCB20" // NotoSansMono-SemiCondensedBold 20pt
#define BACK_GROUND_COLOR	0x2104

#define DRAW_SCREEN_TIMES_COUNT	60

uint16_t colors[] = { TFT_RED, TFT_GREEN, TFT_BLUE, TFT_CYAN, TFT_MAGENTA, TFT_YELLOW, TFT_WHITE, TFT_BLACK };
uint color_idx = 0;


TFT_eSPI tft		= TFT_eSPI();         // Declare object "tft"
TFT_eSprite spr		= TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object
int16_t ScreenWidth;
int16_t ScreenHeight;

float ScreenFPS;
unsigned long DrawScreenTimes[DRAW_SCREEN_TIMES_COUNT] = { 0 };

bool Run_In_Task = true;
TaskHandle_t Screen_flush;
void core0_task(void * pvParameters);



void setup_screen() {
	// (POWER ON)IO15 must be set to HIGH before starting, otherwise the screen will not display when using battery
	pinMode(PIN_POWER_ON, OUTPUT);
	digitalWrite(PIN_POWER_ON, HIGH);

	// Initialise the TFT registers
	tft.begin();
	tft.setAttribute(PSRAM_ENABLE, false);
	tft.setRotation(SCREEN_ROTAION);
	tft.setTextSize(2);
	ScreenWidth = tft.width();
	ScreenHeight = tft.height();

	// Optionally set colour depth to 8 or 16 bits, default is 16 if not spedified
	spr.setColorDepth(8);
	// spr.loadFont(AA_FONT_LARGE); // Must load the font first into the sprite class
	spr.createSprite(ScreenWidth, ScreenHeight);

	spr.setTextColor(TFT_GREEN, BACK_GROUND_COLOR);
	spr.setTextSize(2);
	// Create a sprite of defined size
	spr.fillScreen(TFT_WHITE);
}

void testMaxFPS()
{
	spr.fillSprite(colors[color_idx++]);
	spr.drawString(String(ScreenFPS) + "FPS", 5, 5);

	unsigned long draw_start = micros();

	spr.pushSprite(0, 0);

	unsigned long draw_end = micros();
	unsigned long draw_timespan = draw_end - draw_start;
	Serial.println("Push Sprite Time: " + String(draw_timespan / 1000.0) + "ms\n");


	// draw screen performance
	memmove(&DrawScreenTimes[0], &DrawScreenTimes[1],
			(DRAW_SCREEN_TIMES_COUNT - 1) * sizeof(typeof(DrawScreenTimes[0])));
	DrawScreenTimes[DRAW_SCREEN_TIMES_COUNT - 1] = draw_end;
	if (DrawScreenTimes[0] >= 0)
		ScreenFPS =  (1000000.0 * (DRAW_SCREEN_TIMES_COUNT - 1)) /
						(draw_end - DrawScreenTimes[0]);
}


void setup() {
	Serial.begin(115200);

	setup_screen();

	if (Run_In_Task)
		xTaskCreatePinnedToCore(
			core0_task,
			"screen_flush",
			10000,			/* Stack size in words */
			NULL,			/* Task input parameter */
			0,				/* Priority of the task */
			&Screen_flush,	/* TasScreen_flushk handle. */
			0
		);		/* Core where the task should run */
}

void loop() {
	if (!Run_In_Task)
		testMaxFPS();
}


void core0_task(void * pvParameters) {
	(void) pvParameters;
	for (;;) {
		testMaxFPS();
		vTaskDelay(pdMS_TO_TICKS(1));
	}
}