#include <math.h>

#include <M5StickCPlus2.h>
#include "soc/rtc_cntl_reg.h"
#include "soc/sens_reg.h"

#include <driver/dac.h>
#include <driver/i2s.h>

#define SINFAKT			127.0
#define ADC_MAX_VOLT	3.1
#define VOLT_TO_DAC(v)	((uint8_t)(256 * v / 3.3))

uint32_t buf[128];

char Sinous[] = "Sinous";
char Rectangle[] = "Rectangle";
char Triangle[] = "Triangle";
char *modes[] = { Sinous, Triangle, Rectangle };
uint mode_idx = 0;

uint frequency = 100;
uint freqs[] = { 100, 250, 500, 750, 1000, 1250, 1500, 2000, 2500, 3000, 4000, 5000 };
uint freq_idx = 0;

uint8_t ratio = 50;

bool statChanged = true;
bool initDone = false;

int width = 0;
int height = 0;

i2s_config_t i2s_config = {
	.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
	.sample_rate = 100000,
	.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
	.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
	.communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
	.intr_alloc_flags = 0,
	.dma_buf_count = 2,
	.dma_buf_len = 32,
	.use_apll = 0,
};


void fillBuffer(uint8_t up, uint8_t sz)
{
	uint8_t down;
	uint32_t sample;
	float du, dd, val;
	down = 100 - up;

	uint16_t stup = round(1.0 * sz / 100 * up);
	uint16_t stdwn = round(1.0 * sz / 100 * down);
	uint16_t i;
	if ((stup + stdwn) < sz)
		stup++;

	du = 256.0 / stup;
	dd = 256.0 / stdwn;
	val = 0;
	for (i = 0; i < stup; i++) {
		sample = val;
		sample = sample << 8;
		buf[i] = sample;
		val = val + du;
	}
	val = 255;
	for (i = 0; i < stdwn; i++) {
		sample = val;
		sample = sample << 8;
		buf[i + stup] = sample;
		val = val - dd;
	}
}

void stopAll()
{
	ledcDetachPin(26);
	i2s_driver_uninstall((i2s_port_t)0);
	dac_output_disable(DAC_CHANNEL_2);
	dac_i2s_disable();
	pinMode(26, OUTPUT);
	initDone = false;
	Serial.println("Stopped all");
}

void startRectangle()
{
	ledcAttachPin(26, 2);
	initDone = true;
	Serial.println("Rectangle Started");
}

void rectangleSetFrequency(uint frequency, uint8_t ratio)
{
	ledcSetup(2, frequency, 7);
	ledcWrite(2, 127.0 * ratio / 100);
}

// Dreiecksignal starten
void startTriangle()
{
	i2s_set_pin((i2s_port_t)0, NULL);
	initDone = true;
	Serial.println("Triangle Started");
}

double triangleSetFrequency(uint frequency, uint8_t ratio)
{
	int size = 64;
	if (frequency < 5000)
		size = 64;
	else if (frequency < 10000)
		size = 32;
	else if (frequency < 20000)
		size = 16;
	else
		size = 8;

	uint32_t rate = frequency * 2 * size;

	if (rate < 5200)
		rate = 5200;
	if (rate > 650000)
		rate = 650000;
	frequency = rate / 2 / size;

	i2s_driver_uninstall((i2s_port_t)0);
	i2s_config.sample_rate = rate;
	i2s_config.dma_buf_len = size;
	i2s_driver_install((i2s_port_t)0, &i2s_config, 0, NULL);
	i2s_set_pin((i2s_port_t)0, NULL);
	i2s_set_sample_rates((i2s_port_t)0, rate);
	fillBuffer(ratio, size * 2);
	size_t i2s_Bytes_written;
	i2s_write((i2s_port_t)0, (const char *)&buf, size * 8, &i2s_Bytes_written, 100);
	return frequency;
}

// Sinusausgabe vorbereiten
void startSinus()
{
	dac_output_enable(DAC_CHANNEL_2);
	SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
	SET_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN2_M);
	SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_INV2, 2, SENS_DAC_INV2_S);
	initDone = true;
	Serial.println("Sinus Started");
}

double sinusSetFrequency(uint frequency)
{
	double f, delta, delta_min = 999999999.0;
	uint16_t divi = 0, step = 1, s;
	uint8_t clk_8m_div = 0;
	for (uint8_t div = 1; div < 9; div++) {
		s = round(frequency * div / SINFAKT);
		if ((s > 0) && ((div == 1) || (s < 1024))) {
			f = SINFAKT * s / div;
			/*
				Serial.print(f); Serial.print(" ");
				Serial.print(div); Serial.print(" ");
				Serial.println(s);
				*/
			delta = abs(f - frequency);
			if (delta < delta_min) {
				step = s;
				divi = div - 1;
				delta_min = delta;
			}
		}
	}
	frequency = SINFAKT * step / (divi + 1);
	REG_SET_FIELD(RTC_CNTL_CLK_CONF_REG, RTC_CNTL_CK8M_DIV_SEL, divi);
	SET_PERI_REG_BITS(SENS_SAR_DAC_CTRL1_REG, SENS_SW_FSTEP, step, SENS_SW_FSTEP_S);
	return frequency;
}

void controlGenerator()
{
	stopAll();

	switch (mode_idx)
	{
		case 0:
			if (!initDone)
				startSinus();
			frequency = sinusSetFrequency(frequency);
			break;

		case 1:
			if (!initDone)
				startTriangle();
			frequency = triangleSetFrequency(frequency, ratio);
			break;

		case 2:
			if (!initDone)
				startRectangle();
			rectangleSetFrequency(frequency, ratio);
			break;
	}
}

void printSettings()
{
	M5.Lcd.fillRect(0, 20, width, height - 20, BLACK);
	M5.Lcd.setCursor(4, 34);

	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("Mode:  ");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %s\n", modes[mode_idx]);

	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("Freq:  ");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %u\n", frequency);

	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.printf("Ratio: ");
	M5.Lcd.setTextColor(ORANGE);
	M5.Lcd.printf(" %d\n", ratio);
}

void setup()
{
	M5.begin();

	Serial.begin(115200);
	Serial.println("Use Pin 26");
	Serial.print("Commands: M (Mode): S (Sinus), T (Triangle), R (Rectangle), F (Frequency), R (Ratio)");

	M5.Lcd.setRotation(3);
	width = M5.Lcd.width();
	height = M5.Lcd.height();

	M5.Lcd.setTextSize(2); 
	M5.Lcd.setCursor(4, 4);
	M5.Lcd.setTextColor(GREEN);
	M5.Lcd.println("Waveform Generator");
	// M5.Lcd.printf("%d, %d", (sizeof(modes) / sizeof(typeof(modes[0]))), (sizeof(freqs) / sizeof(typeof(freqs[0]))));
}

void loop()
{
	char mode;

	M5.update();  // Read the press state of the key.  读取按键 A, B, C 的状态
	if (M5.BtnA.wasReleased()) {
		mode_idx = (mode_idx + 1) % (sizeof(modes) / sizeof(typeof(modes[0])));
		statChanged = true;
	} else if (M5.BtnB.wasReleased()) {
		freq_idx = (freq_idx + 1) % (sizeof(freqs) / sizeof(typeof(freqs[0])));
		frequency = freqs[freq_idx];
		statChanged = true;
	}

	if (Serial.available() > 0) {
		String inp = Serial.readStringUntil('\n');
		char cmd = inp[0];
		String dat = inp.substring(2);

		Serial.println("cmd: " + String(cmd) + ", data: " + dat);

		switch (cmd)
		{
			case 'M':
			case 'm':
				mode = dat[0];
				break;
			case 'F':
			case 'f':
				frequency = dat.toDouble();
				break; // Frequency
			case 'R':
			case 'r':
				ratio = dat.toInt();
				break; // Ratio
		}
		switch (mode)
		{
		case 'S':
		case 's':
			mode_idx = 0;
			break;
		
		case 'T':
		case 't':
			mode_idx = 1;
			break;
	
		case 'R':
		case 'r':
			mode_idx = 2;
			break;
		
		default:
			break;
		}
		if (ratio > 100)
			ratio = 100;
		if (frequency < 20)
			frequency = 20;
		if (frequency > 20000)
			frequency = 20000;

		statChanged = true;
	}

	if (statChanged) {
		controlGenerator();
		String ba;
		switch (mode)
		{
			case 'S':
			case 's':
				ba = "Sine";
				break;
			case 'T':
			case 't':
				ba = " Triangle";
				break;
			case 'R':
			case 'r':
				ba = "Rectangle";
				break;
		}

		Serial.println("**************** Adjusted Values / Eingestellte Werte ********************");
		Serial.print("Mode		= ");
		Serial.println(ba);
		Serial.print("Frequncy	= ");
		Serial.print(frequency);
		Serial.println("Hz");
		Serial.print("Ratio		= ");
		Serial.print(ratio);
		Serial.println("%");
		Serial.println();
		Serial.print("Commands / Kommandos: M (Mode), F (Frequency), R (Ratio) : ");

		printSettings();

		statChanged = false;
	}
}