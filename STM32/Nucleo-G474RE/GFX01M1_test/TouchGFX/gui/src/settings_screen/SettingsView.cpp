#include <gui/settings_screen/SettingsView.hpp>

#include <gui/common/Constants.hpp>
#include <gui/common/Enums.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

SettingsView::SettingsView():
    tickCounter(0),     // Used for the animations
    clockCounter(0),    // Used for the clock tick count
    secondCounter(0),   // Initial clock arbitrary set to 10:20 in model.c , overwritten by the settings if any
    minuteCounter(0),
    hourCounter(0),
    selectedOption(0),   // "0" refers to the Return option in Enums.hpp
    numberOptions(3),    // Amount of options according to the SettingOptions enum
    selectedLanguage(Texts::getLanguage()),   // "0" refers to the English option in Enums.hpp
    numberLanguage(NUMBER_OF_LANGUAGES),    // Amount of options according to the LanguageOptions enum
    previousScreen(0),
    minuteTimer(0),
    secondTimer(0),
    isEnterScreenAnimationFinished(false),
    isEndingAnimationCalled(false),
    animationState(ANIMATION_STEP_0),
    exitAnimationState(EXIT_ANIMATION_STEP_0),
    highlightedItem(BACK_BUTTON)
{

}

void SettingsView::setupScreen()
{
    readSharedData();
    customEntrySetup();

    SettingsViewBase::setupScreen();
}

void SettingsView::tearDownScreen()
{
    saveSharedData();

    SettingsViewBase::tearDownScreen();
}



/*******************************************************
            Handlers and hardware buttons
*******************************************************/


void SettingsView::handleTickEvent()
{
    tickCounter++;

    // Animations

    if (previousScreen == Enums::Cooking)
    {
        if (tickCounter >= 60)  // 60 ticks = 1s
        {
            tickCounter = 0;

            updateTimer();
        }
    }

    if (!isEnterScreenAnimationFinished)
    {
        ENTERSCREEN_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    if (isEndingAnimationCalled)
    {
        EXITSCREEN_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    // Clock
    clockAnimation();
}

void SettingsView::pressUpKeyFunction()
{
    if ((isEnterScreenAnimationFinished) && (!isEndingAnimationCalled))
    {
        switch (highlightedItem)
        {
        case SettingsView::BACK_BUTTON:
            // Update back button
            returnButton.setBitmaps(BITMAP_LEFT_BUTTON_BACKGROUND_ID, BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID);
            returnButton.invalidate();

            // Update clock button
            hourDateOptionSettings.setColor(Color::getColorFromRGB(255, 255, 255));
            hourDateOptionSettings.invalidate();

            highlightedItem = CLOCK_BUTTON;
            break;
        case SettingsView::LANGUAGE_BUTTON:
            // Update back button
            returnButton.setBitmaps(BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID, BITMAP_LEFT_BUTTON_BACKGROUND_ID);
            returnButton.invalidate();

            // Update language button
            languageOptionsSettingsTextArea.setColor(touchgfx::Color::getColorFromRGB(50, 255, 238));
            languageOptionsSettingsTextArea.invalidate();

            highlightedItem = BACK_BUTTON;
            break;
        case SettingsView::CLOCK_BUTTON:
            // Update language button
            languageOptionsSettingsTextArea.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
            languageOptionsSettingsTextArea.invalidate();

            // Update clock button
            hourDateOptionSettings.setColor(Color::getColorFromRGB(50, 255, 238));
            hourDateOptionSettings.invalidate();

            highlightedItem = LANGUAGE_BUTTON;
            break;
        case SettingsView::LANGAUGE_SELECTION:
            // Change selected language
            selectedLanguage--;
            languageSettingsOption();
            break;
        case SettingsView::HOUR_SETTING:
        case SettingsView::MINUTE_SETTING:
            increaseClockSettings();
            break;
        default:
            break;
        }
    }
}

void SettingsView::pressDownKeyFunction()
{
    if (isEnterScreenAnimationFinished)
    {
        switch (highlightedItem)
        {
        case SettingsView::BACK_BUTTON:
            // Update back button
            returnButton.setBitmaps(BITMAP_LEFT_BUTTON_BACKGROUND_ID, BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID);
            returnButton.invalidate();

            // Update language button
            languageOptionsSettingsTextArea.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
            languageOptionsSettingsTextArea.invalidate();

            highlightedItem = LANGUAGE_BUTTON;
            break;
        case SettingsView::LANGUAGE_BUTTON:
            // Update language button
            languageOptionsSettingsTextArea.setColor(touchgfx::Color::getColorFromRGB(50, 255, 238));
            languageOptionsSettingsTextArea.invalidate();

            // Update clock button
            hourDateOptionSettings.setColor(Color::getColorFromRGB(255, 255, 255));
            hourDateOptionSettings.invalidate();

            highlightedItem = CLOCK_BUTTON;
            break;
        case SettingsView::CLOCK_BUTTON:
            // Update back button
            returnButton.setBitmaps(BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID, BITMAP_LEFT_BUTTON_BACKGROUND_ID);
            returnButton.invalidate();

            // Update clock button
            hourDateOptionSettings.setColor(Color::getColorFromRGB(50, 255, 238));
            hourDateOptionSettings.invalidate();

            highlightedItem = BACK_BUTTON;
            break;
        case SettingsView::LANGAUGE_SELECTION:
            // Change selected language
            selectedLanguage++;
            languageSettingsOption();
            break;
        case SettingsView::HOUR_SETTING:
        case SettingsView::MINUTE_SETTING:
            decreaseClockSettings();
            break;
        default:
            break;
        }
    }
}

void SettingsView::pressCenterKeyFunction()
{
    //Different actions depending on the current option
    if ((isEnterScreenAnimationFinished) && (!isEndingAnimationCalled))
    {
        switch (highlightedItem)
        {
        case SettingsView::BACK_BUTTON:
            // Exit screen with animation
            isEndingAnimationCalled = true;
            break;
        case SettingsView::LANGUAGE_BUTTON:
            // Enter language option change view
            languageOptionsSettingsTextArea.setVisible(false);
            languageOptionsTextArea.setVisible(true);
            languageOptionsSettingsTextArea.invalidate();
            languageOptionsTextArea.invalidate();

            highlightedItem = LANGAUGE_SELECTION;
            break;
        case SettingsView::CLOCK_BUTTON:
            // Enter clock option change view
            hourSettingTextArea.setColor(touchgfx::Color::getColorFromRGB(240, 175, 12));
            minuteSettingTextArea.setColor(touchgfx::Color::getColorFromRGB(154, 188, 255));
            hourDateOptionSettings.setVisible(false);
            hourSettingTextArea.setVisible(true);
            minuteSettingTextArea.setVisible(true);
            hourDateOptionSettings.invalidate();

            clockClickListener.setTouchable(false);
            hourClickListener.setTouchable(true);
            minuteClickListener.setTouchable(true);

            highlightedItem = HOUR_SETTING;
            break;
        case SettingsView::LANGAUGE_SELECTION:
            // Confirm language selection
            languageOptionsSettingsTextArea.setVisible(true);
            languageOptionsTextArea.setVisible(false);
            languageOptionsSettingsTextArea.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
            languageOptionsSettingsTextArea.invalidate();
            languageOptionsTextArea.invalidate();

            buttonContainer.setVisible(false);
            buttonContainer.invalidate();

            highlightedItem = LANGUAGE_BUTTON;
            break;
        case SettingsView::HOUR_SETTING:
            // Confirm hour setting and go to minute setting
            hourSettingTextArea.setColor(touchgfx::Color::getColorFromRGB(154, 188, 255));
            minuteSettingTextArea.setColor(touchgfx::Color::getColorFromRGB(240, 175, 12));
            hourSettingTextArea.invalidate();
            minuteSettingTextArea.invalidate();

            highlightedItem = MINUTE_SETTING;
            break;
        case SettingsView::MINUTE_SETTING:
            // Confirm minute setting and go to clock title view
            minuteSettingTextArea.setColor(touchgfx::Color::getColorFromRGB(154, 188, 255));
            hourDateOptionSettings.setVisible(true);
            hourSettingTextArea.setVisible(false);
            minuteSettingTextArea.setVisible(false);
            hourDateOptionSettings.invalidate();

            clockClickListener.setTouchable(true);
            hourClickListener.setTouchable(false);
            minuteClickListener.setTouchable(false);

            buttonContainer.setVisible(false);
            buttonContainer.invalidate();

            highlightedItem = CLOCK_BUTTON;
            break;
        default:
            break;
        }
    }
}

void SettingsView::languageButtonClicked()
{
    minuteSettingTextArea.setVisible(false);
    hourSettingTextArea.setVisible(false);
    hourDateOptionSettings.setVisible(true);
    hourDateOptionSettings.invalidate();

    clockClickListener.setTouchable(true);
    hourClickListener.setTouchable(false);
    minuteClickListener.setTouchable(false);

    highlightedItem = LANGUAGE_BUTTON;
    pressCenterKeyFunction();

    // Show buttons if language is clicked by touch
    buttonContainer.invalidate();
    buttonContainer.setXY(Constants::X_languageSettings_buttonContainer, Constants::Y_languageSettings_buttonContainer);
    buttonContainer.setVisible(true);
    buttonContainer.invalidate();

    // Reset back button
    returnButton.setBitmaps(BITMAP_LEFT_BUTTON_BACKGROUND_ID, BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID);
    returnButton.invalidate();

    // Reset clock button
    minuteSettingTextArea.setVisible(false);
    hourSettingTextArea.setVisible(false);
    hourDateOptionSettings.setVisible(true);
    hourDateOptionSettings.setColor(Color::getColorFromRGB(50, 255, 238));
    hourDateOptionSettings.invalidate();
}

void SettingsView::clockButtonClicked()
{
    languageOptionsSettingsTextArea.setVisible(true);
    languageOptionsTextArea.setVisible(false);
    languageOptionsSettingsTextArea.invalidate();

    highlightedItem = CLOCK_BUTTON;
    pressCenterKeyFunction();

    // Show buttons if clock is clicked by touch
    buttonContainer.invalidate();
    buttonContainer.setXY(Constants::X_clockSettings_buttonContainer, Constants::Y_clockSettings_buttonContainer);
    buttonContainer.setVisible(true);
    buttonContainer.invalidate();

    // Reset back button
    returnButton.setBitmaps(BITMAP_LEFT_BUTTON_BACKGROUND_ID, BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID);
    returnButton.invalidate();

    // Reset language button
    languageOptionsTextArea.setVisible(false);
    languageOptionsSettingsTextArea.setVisible(true);
    languageOptionsSettingsTextArea.setColor(Color::getColorFromRGB(50, 255, 238));
    languageOptionsSettingsTextArea.invalidate();
}

void SettingsView::hourIsClicked()
{
    highlightedItem = CLOCK_BUTTON;
    pressCenterKeyFunction();

    // Show buttons if clock is clicked by touch
    buttonContainer.invalidate();
    buttonContainer.setXY(Constants::X_clockSettings_buttonContainer, Constants::Y_clockSettings_buttonContainer);
    buttonContainer.setVisible(true);
    buttonContainer.invalidate();
}

void SettingsView::minuteIsClicked()
{
    highlightedItem = HOUR_SETTING;
    pressCenterKeyFunction();

    // Show buttons if clock is clicked by touch
    buttonContainer.invalidate();
    buttonContainer.setXY(Constants::X_clockSettings_buttonContainer, Constants::Y_clockSettings_buttonContainer);
    buttonContainer.setVisible(true);
    buttonContainer.invalidate();
}

void SettingsView::returnButtonIsClicked()
{
    // Reset language button
    languageOptionsTextArea.setVisible(false);
    languageOptionsSettingsTextArea.setVisible(true);
    languageOptionsSettingsTextArea.setColor(Color::getColorFromRGB(50, 255, 238));
    languageOptionsSettingsTextArea.invalidate();

    // Reset clock button
    minuteSettingTextArea.setVisible(false);
    hourSettingTextArea.setVisible(false);
    hourDateOptionSettings.setVisible(true);
    hourDateOptionSettings.setColor(Color::getColorFromRGB(50, 255, 238));
    hourDateOptionSettings.invalidate();

    // Hide button container
    buttonContainer.setVisible(false);
    buttonContainer.invalidate();

    // Exit screen with animation
    isEndingAnimationCalled = true;
}

/*******************************************************
            UI animations and interactions
*******************************************************/

void SettingsView::customEntrySetup()
{
    minuteClickListener.setTouchable(false);
    hourClickListener.setTouchable(false);

    setUpTheClock();

    returnButton.setBitmaps(BITMAP_LEFT_BUTTON_WHITE_BACKGROUND_ID, BITMAP_LEFT_BUTTON_BACKGROUND_ID);
}

void SettingsView::setUpTheClock()
{
    hourCounter = getHourClock();
    Unicode::snprintf(hourSettingTextAreaBuffer, HOURSETTINGTEXTAREA_SIZE, "%02d", hourCounter);

    minuteCounter = getMinuteClock();
    Unicode::snprintf(minuteSettingTextAreaBuffer, MINUTESETTINGTEXTAREA_SIZE, "%02d", minuteCounter);
}

void SettingsView::ENTERSCREEN_STATEMACHINE()
{
    if (animationState > ANIMATION_READY)
    {

        if (animationState == ANIMATION_STEP_0) // fade in the title
        {
            titleTextArea.startFadeAnimation(255, 15);
            animationState = ANIMATION_STEP_1;

        }
        else if (animationState == ANIMATION_STEP_1)    // fade in the background images
        {
            if (!titleTextArea.isFadeAnimationRunning())    // We wait for the selected element image to finish fading in before moving to the next step
            {
                selectedLanguageBackgroundLeft.setAlpha(255);
                selectedLanguageBackgroundMiddle.setAlpha(255);
                selectedLanguageBackgroundRight.setAlpha(255);
                selectedClockBackgroundLeft.setAlpha(255);
                selectedClockBackgroundMiddle.setAlpha(255);
                selectedClockBackgroundRight.setAlpha(255);
                selectedLanguageBackground.invalidate();
                selectedClockBackground.invalidate();
                animationState = ANIMATION_STEP_2;
            }
        }
        else if (animationState == ANIMATION_STEP_2)    // fade in the remaining elements
        {
            hourDateOptionSettings.startFadeAnimation(255, 20);
            languageOptionsSettingsTextArea.startFadeAnimation(255, 20);
            animationState = ANIMATION_DONE;

        }
        else if (animationState == ANIMATION_DONE)    // wait for end of fading to truly be done with the enter screen animation
        {
            if (!languageOptionsSettingsTextArea.isFadeAnimationRunning())   // We wait for the moving up animation to finish before moving to the next step
            {
                isEnterScreenAnimationFinished = true;
            }
        }
    }
}


void SettingsView::increaseClockSettings()
{
    if (highlightedItem == HOUR_SETTING)   // check which time values we wish to set
    {
        hourCounter = (hourCounter + 1) % 24;

        Unicode::snprintf(hourSettingTextAreaBuffer, HOURSETTINGTEXTAREA_SIZE, "%02d", hourCounter);
        hourSettingTextArea.invalidate();
    }
    else if (highlightedItem == MINUTE_SETTING)
    {
        minuteCounter = (minuteCounter + 1) % 60;

        Unicode::snprintf(minuteSettingTextAreaBuffer, MINUTESETTINGTEXTAREA_SIZE, "%02d", minuteCounter);
        minuteSettingTextArea.invalidate();
    }
}

void SettingsView::decreaseClockSettings()
{
    if (highlightedItem == HOUR_SETTING)   // check which time values we wish to set
    {
        hourCounter--;

        if (hourCounter < 0)
        {
            hourCounter = 23;
        }

        Unicode::snprintf(hourSettingTextAreaBuffer, HOURSETTINGTEXTAREA_SIZE, "%02d", hourCounter);
        hourSettingTextArea.invalidate();
    }
    else if (highlightedItem == MINUTE_SETTING)
    {
        minuteCounter--;

        if (minuteCounter < 0)
        {
            minuteCounter = 59;
        }

        Unicode::snprintf(minuteSettingTextAreaBuffer, MINUTESETTINGTEXTAREA_SIZE, "%02d", minuteCounter);
        minuteSettingTextArea.invalidate();
    }
}

void SettingsView::languageSettingsOption()
{
    if (selectedLanguage < 0)
    {
        selectedLanguage = numberLanguage - 1;
    }
    else if (selectedLanguage > numberLanguage - 1)
    {
        selectedLanguage = 0;
    }

    Texts::setLanguage(selectedLanguage);

    hourDateOptionSettings.invalidate();
    languageOptionsTextArea.invalidate();
    titleTextArea.invalidate();
}



void SettingsView::EXITSCREEN_STATEMACHINE()
{
    if (exitAnimationState > EXIT_ANIMATION_READY)
    {

        if (exitAnimationState == EXIT_ANIMATION_STEP_0) // fade out the text
        {
            hourDateOptionSettings.startFadeAnimation(0, 15);
            languageOptionsSettingsTextArea.startFadeAnimation(0, 15);
            exitAnimationState = EXIT_ANIMATION_STEP_1;

        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_1)    // fade out the background images
        {
            if (!hourDateOptionSettings.isFadeAnimationRunning())    // We wait for the selected element image to finish fading in before moving to the next step
            {
                selectedLanguageBackgroundLeft.setAlpha(0);
                selectedLanguageBackgroundMiddle.setAlpha(0);
                selectedLanguageBackgroundRight.setAlpha(0);
                selectedClockBackgroundLeft.setAlpha(0);
                selectedClockBackgroundMiddle.setAlpha(0);
                selectedClockBackgroundRight.setAlpha(0);
                selectedLanguageBackground.invalidate();
                selectedClockBackground.invalidate();
                exitAnimationState = EXIT_ANIMATION_STEP_2;
            }
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_2)    // fade out the title
        {
            titleTextArea.startFadeAnimation(0, 15);
            if (previousScreen == Enums::Settings)
            {
                returnButton.startFadeAnimation(0, 15);
            }
            exitAnimationState = EXIT_ANIMATION_STEP_3;
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_3)    // fade in the remaining elements
        {
            if (!titleTextArea.isFadeAnimationRunning())   // We wait for the longuest animation to finish before moving to the next step
            {
                if (previousScreen == Enums::Cooking)
                {
                    goToCookingMenu();
                }
                else
                {
                    goToMainMenu();
                }
                exitAnimationState = EXIT_ANIMATION_DONE;
            }
        }
    }
}

void SettingsView::updateTimer() // only used if we were in the cooking screen before to keep the timer running
{
    if ((minuteTimer != 0) || (secondTimer != 0))        // If the timer is not over
    {
        secondTimer = secondTimer - 1;

        if (secondTimer < 0)    // if the second timer goes below 0, we decrease one minute and go back to 59s
        {
            secondTimer = 59;

            if (minuteTimer > 0)    // make sure we are not in the negative minutes
            {
                minuteTimer = minuteTimer - 1;
            }
        }
    }
    else
    {
        goToCookingMenu();
    }
}

void SettingsView::clockAnimation()
{
    clockCounter++;

    if (clockCounter == 60)   // 60 ticks is equal to 1s
    {
        clockCounter = 0;

        secondCounter++;    // we increment the second counter to +1s

        /*** For the purpose of the demo 60s will be reduced to 10s to see the clock evolution ***/
        if (secondCounter > 10)
        {
            secondCounter = 0; // if 60 seconds haved passed we increment the minute counter and reset the second timer (=clockCounter) to 0

            minuteCounter++;

            if (minuteCounter > 59)
            {
                minuteCounter = 0;  // if 60 minutes have passed we increment the hour timer and reset the minute timer to 0

                hourCounter++;

                if (hourCounter > 23)
                {
                    hourCounter = 0; // if 24 hours have passed we reset the hour timer to 0
                }

                Unicode::snprintf(hourSettingTextAreaBuffer, HOURSETTINGTEXTAREA_SIZE, "%02d", hourCounter);
                hourSettingTextArea.invalidate();
            }

            Unicode::snprintf(minuteSettingTextAreaBuffer, MINUTESETTINGTEXTAREA_SIZE, "%02d", minuteCounter);
            minuteSettingTextArea.invalidate();
        }
    }
}

void SettingsView::readSharedData()
{
    previousScreen = getPreviousScreen();
    minuteTimer = getMinuteCookingTimer();
    secondTimer = getSecondCookingTimer();
}

void SettingsView::saveSharedData()
{
    saveHourClock(hourCounter);
    saveMinuteClock(minuteCounter);
    saveMinuteCookingTimer(minuteTimer);
    saveSecondCookingTimer(secondTimer);
}
