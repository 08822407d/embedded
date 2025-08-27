namespace Constants
{
/*******************************************************
            Selection menu Constants
*******************************************************/
static const int X_bottomContainer = 35;
static const int Y_starting_bottomContainer = 240;
static const int Y_ending_bottomContainer = 180;
static const int duration_moveAnimation_bottomContainer = 20;
static const int X_starting_righButton = 258;
static const int X_ending_rightButton = X_starting_righButton + 10;
static const int Y_righButton = 94;
static const int X_starting_leftButton = 42;
static const int X_ending_leftButton = X_starting_leftButton - 10;
static const int Y_leftButton = 94;

/*******************************************************
            Setting menu Constants
*******************************************************/
static const int RedColorValue_SelectedElement = 50;
static const int GreenColorValue_SelectedElement = 255;
static const int BlueColorValue_SelectedElement = 238;
static const int RedColorValue_OverviewedElement = 255;
static const int GreenColorValue_OverviewedElement = 255;
static const int BlueColorValue_OverviewedElement = 255;
static const int X_languageSettings_buttonContainer = 283;
static const int Y_languageSettings_buttonContainer = 69;
static const int X_clockSettings_buttonContainer = 283;
static const int Y_clockSettings_buttonContainer = 144;
static const int X_hourSettings_buttonContainer = 98;
static const int Y_hourSettings_buttonContainer = 144;
static const int X_minuteSettings_buttonContainer = 244;
static const int Y_minuteSettings_buttonContainer = 144;


/*******************************************************
            Cooking Selection Constants
*******************************************************/
static const float fakeCookingTimeTicks = 3600.0f; // in ticks, 3600 ticks = 1 min (= 60ticks * 60s)
static const float circleIncrementationPerTick = 360.0f / fakeCookingTimeTicks;  // value to increment every second to the 360degree circle based on the total cooking time

static const int minuteRice = 30;
static const int secondRice = 00;
static const int minutePancake = 12;
static const int secondPancake = 00;
static const int minuteDumplings = 70;
static const int secondDumplings = 00;
static const int minuteFish = 28;
static const int secondFish = 00;
static const int RedColorValue_UnselectedMenu = 40;
static const int GreenColorValue_UnselectedMenu = 0;
static const int BlueColorValue_UnselectedMenu = 96;
static const int RedColorValue_OverviewedMenu = 80;
static const int GreenColorValue_OverviewedMenu = 0;
static const int BlueColorValue_OverviewedMenu = 192;
}
