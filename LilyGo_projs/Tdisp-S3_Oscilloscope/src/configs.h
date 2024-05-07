/*================================================================================*
 *								hand set configurations							  *
 *================================================================================*/

#define Enable_longPress_Continuous
#define WAVE_INPUT_PIN		    1
#define SCREEN_ROTAION			3
#define EXPECT_FPS				30

#ifdef AMOLED
#  define SPR_TXT_SIZE			2
#  define GRID_SIZE				60
#else
#  define SPR_TXT_SIZE			1
#  define GRID_SIZE				40
#endif

#define BG_DARK_GRAY	        0x2104
#define BACK_GROUND_COLOR		BG_DARK_GRAY

#define ADC_SAMPLE_RATE			80000
// #define ADC_SAMPLE_RATE			SOC_ADC_SAMPLE_FREQ_THRES_HIGH
#define ONCE_SAMPLES		    256UL



/*================================================================================*
 *								auto calc configurations						  *
 *================================================================================*/

#define MAX_SAMPLE_COUNT		((ADC_SAMPLE_RATE / EXPECT_FPS))



/*================================================================================*
 *							    	hardware constants							  *
 *================================================================================*/

#define ADC_READ_MAX_VAL		4095
// #define ADC_READ_MAX_VAL		8191
#define ADC_VOLTREAD_CAP		3100