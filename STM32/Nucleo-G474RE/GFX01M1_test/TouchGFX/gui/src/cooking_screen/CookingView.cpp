#include <gui/cooking_screen/CookingView.hpp>

#include <gui/common/Constants.hpp>
#include <gui/common/Enums.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

#include <touchgfx/Utils.hpp>

CookingView::CookingView():
    elementID(0),
    minuteTimer(0), // used for the remaining cooking time and animation (circle value is in degrees)
    secondTimer(0),
    circleEndValue(0.0),
    circleIndicatorFillerSteps(0.0),
    tickCounter(0),
    clockCounter(0),    // Used for the clock tick count
    secondCounter(0),   // Initial clock arbitrary set to 10:20 in model.c , overwritten by the settings if any
    minuteCounter(0),
    hourCounter(0),
    settingMenuSelected(false),
    informationgMenuSelected(false),
    cancelOptionSelected(false),
    isCookingFinished(false),
    isEnterScreenAnimationFinished(false),
    isEndingAnimationCalled(false),
    animationState(ANIMATION_STEP_0),
    exitAnimationState(EXIT_ANIMATION_STEP_0),
    hasCenterKeyBeenPressed(false)
{

    /*******************************************************
                Make sure the screen is clean of unecessary elements
    *******************************************************/

}

void CookingView::setupScreen()
{
    CookingViewBase::setupScreen();

    setUpTheClock();

    customSetupScreen();
}

void CookingView::tearDownScreen()
{
    CookingViewBase::tearDownScreen();

    saveSharedData();
}


/*******************************************************
            Handlers and hardware buttons
*******************************************************/

void CookingView::handleTickEvent()
{
    tickCounter++;

    // Animations

    if (!isEnterScreenAnimationFinished)
    {
        ENTERSCREEN_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    if (isEndingAnimationCalled)
    {
        EXITSCREEN_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    if ((isEnterScreenAnimationFinished) && (!isCookingFinished) && (!isEndingAnimationCalled)) // we update this every x tick to have a smoother animation
    {
        updateCircleTimer();    // For the simulation we will distinguish the cooking timer and the circle indicator to speed up the cooking process to 1 minute instead of x minutes (see constants.cpp to change the duration)
    }

    if ((isEnterScreenAnimationFinished) && (tickCounter >= 60) && (!isEndingAnimationCalled))  // every 60 ticks = 1s
    {

        if (!isCookingFinished)
        {
            tickCounter = 0;

            updateTimer();
        }
        else
        {
            if (tickCounter >= 120) // we wait x ticks before going back to the menu selection once we have finished cooking
            {
                pressCenterKeyFunction();
            }
        }
    }
    // Clock, independant of the tickCounter variable to prevent any issues
    clockAnimation();
}

void CookingView::pressCenterKeyFunction()
{
    if (isEnterScreenAnimationFinished && !hasCenterKeyBeenPressed)   // prevent spamming the center key
    {
        circleIndicatorFillerSteps = circleEndValue / 36; // reduce the circle step by step based on the current value of the circlce. Devide the current value by the number of ticks you wish to have the circle animation completely finish

        hasCenterKeyBeenPressed = true;

        if (settingMenuSelected)
        {
            goToSettingMenu();
        }
        else if (informationgMenuSelected)
        {
            goToInformationMenu();
        }
        else
        {
            isEndingAnimationCalled = true;
        }
    }
}

void CookingView::pressUpKeyFunction()
{
    if (isEnterScreenAnimationFinished)   // wait for the enter animation to be finished
    {
        if (cancelOptionSelected)
        {
            cancelOptionSelected = false;
            changeColorUnselectedMenuBackground();
        }
        else if ((!settingMenuSelected) || (informationgMenuSelected))      // No need to update the setting background image again if it the settings option is already selected
        {
            informationgMenuSelected = true;    // the first selected option will always be the information menu

            changeColorOverviewedMenuBackground();
        }
    }
}

void CookingView::pressDownKeyFunction()
{
    if (isEnterScreenAnimationFinished)   // wait for the enter animation to be finished
    {
        if ((settingMenuSelected) || (informationgMenuSelected))      // If we are in the upper menu we go back to the normal state
        {
            settingMenuSelected = false;
            informationgMenuSelected = false;

            changeColorUnselectedMenuBackground();
        }
        else if (!cancelOptionSelected) // if we are in the normal state we go to the cancel option highlight
        {
            cancelOptionSelected = true;
            changeColorOverviewedMenuBackground();
        }
    }
}

void CookingView::pressRightKeyFunction()
{
    if (isEnterScreenAnimationFinished)
    {
        if (settingMenuSelected)        // If we are not in the cooking selection option, the right and left keys will be used to navigaate to the information and setting options
        {
            informationgMenuSelected = true;
            settingMenuSelected = false;
        }
        else if (informationgMenuSelected)
        {
            informationgMenuSelected = false;
            settingMenuSelected = true;
        }

        changeColorOverviewedMenuBackground();
        changeColorUnselectedMenuBackground();
    }
}

void CookingView::pressLeftKeyFunction()
{
    if (isEnterScreenAnimationFinished)   // wait for the enter animation to be finished
    {
        if (settingMenuSelected)        // If we are not in the cooking selection option, the right and left keys will be used to navigaate to the information and setting options
        {
            informationgMenuSelected = true;
            settingMenuSelected = false;
        }
        else if (informationgMenuSelected)
        {
            informationgMenuSelected = false;
            settingMenuSelected = true;
        }

        changeColorOverviewedMenuBackground();
        changeColorUnselectedMenuBackground();
    }
}



/*******************************************************
            UI animations and interactions
*******************************************************/
void CookingView::setUpTheClock()
{
    hourCounter = getHourClock();
    minuteCounter = getMinuteClock();
    touchgfx::Unicode::snprintf(clockTextAreaBuffer1, CLOCKTEXTAREABUFFER1_SIZE, "%02d", hourCounter);
    touchgfx::Unicode::snprintf(clockTextAreaBuffer2, CLOCKTEXTAREABUFFER2_SIZE, "%02d", minuteCounter);
}
void CookingView::customSetupScreen()
{
    elementID = getElementID();      // We get the ID of the element to cook
    setUpInfoAboutCookingFood();

    hasCenterKeyBeenPressed = false;

    if (getPreviousScreen() == Enums::Cooking)    // If we were previously in the cooking screen before going to the info or setting menus, we collect the remaining time
    {
        minuteTimer = getMinuteCookingTimer();
        secondTimer = getSecondCookingTimer();
        circleEndValue = (60 - secondTimer) * 60 * Constants::circleIncrementationPerTick;

        if (minuteTimer <= CookingView::cookingTimeMinutes - 2)  // Check if  the cooking is already done
        {
            isCookingFinished = true;
            minuteTimer = 0;
            secondTimer = 0;
            circleEndValue = 3600 * Constants::circleIncrementationPerTick;
        }

        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer1, COOKINGTIMETEXTAREABUFFER1_SIZE, "%02d", minuteTimer);
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer2, COOKINGTIMETEXTAREABUFFER2_SIZE, "%02d", secondTimer);
        cookingCircle.updateArcEnd(circleEndValue);
    }
    else
    {
        cookingCircle.setArc(0, 0);
    }

    CookingView::invalidate();
}

int16_t CookingView::cookingTimeMinutes = 0;

void CookingView::setUpInfoAboutCookingFood()
{
    switch (elementID)      // Change the image, name and cooking time for the selected element
    {
    case Enums::Rice:       // Rice = 0
        selectedImage.setBitmap(BITMAP_RICE_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_SELECTEDWHITERICE));
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer1, COOKINGTIMETEXTAREABUFFER1_SIZE, "%02d", Constants::minuteRice);
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer2, COOKINGTIMETEXTAREABUFFER2_SIZE, "%02d", Constants::secondRice);
        minuteTimer = Constants::minuteRice;
        CookingView::cookingTimeMinutes = Constants::minuteRice;
        secondTimer = Constants::secondRice;
        break;
    case Enums::Pancake:
        selectedImage.setBitmap(BITMAP_PANCAKES_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_SELECTEDPANCAKES));
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer1, COOKINGTIMETEXTAREABUFFER1_SIZE, "%02d", Constants::minutePancake);
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer2, COOKINGTIMETEXTAREABUFFER2_SIZE, "%02d", Constants::secondPancake);
        minuteTimer = Constants::minutePancake;
        CookingView::cookingTimeMinutes = Constants::minutePancake;
        secondTimer = Constants::secondPancake;
        break;
    case Enums::Dumpling:
        selectedImage.setBitmap(BITMAP_ICON_DUMPLINGS_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_SELECTEDDUMPLINGS));
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer1, COOKINGTIMETEXTAREABUFFER1_SIZE, "%02d", Constants::minuteDumplings);
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer2, COOKINGTIMETEXTAREABUFFER2_SIZE, "%02d", Constants::secondDumplings);
        minuteTimer = Constants::minuteDumplings;
        CookingView::cookingTimeMinutes = Constants::minuteDumplings;
        secondTimer = Constants::secondDumplings;
        break;
    case Enums::Fish:
        selectedImage.setBitmap(BITMAP_FISH_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_SELECTEDFISH));
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer1, COOKINGTIMETEXTAREABUFFER1_SIZE, "%02d", Constants::minuteFish);
        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer2, COOKINGTIMETEXTAREABUFFER2_SIZE, "%02d", Constants::secondFish);
        minuteTimer = Constants::minuteFish;
        CookingView::cookingTimeMinutes = Constants::minuteFish;
        secondTimer = Constants::secondFish;
        break;
    }
}

void CookingView::ENTERSCREEN_STATEMACHINE()
{
    if (animationState > ANIMATION_READY)
    {

        if (animationState == ANIMATION_STEP_0) // enter screen animation: timer and cancel button fade in
        {
            timerImage.startFadeAnimation(255, 10);
            cancelButton.startFadeAnimation(255, 10);
            animationState = ANIMATION_STEP_1;
        }
        else if (animationState == ANIMATION_STEP_1)    // cooking background appears by fading out the fake center circle
        {
            if (!cancelButton.isFadeAnimationRunning())
            {
                animationState = ANIMATION_STEP_2;
            }
        }
        else if (animationState == ANIMATION_STEP_2)    // option text and timer fade in
        {
            cookingTimeTextArea.startFadeAnimation(255, 15);
            optionTextArea.startFadeAnimation(255, 15);
            animationState = ANIMATION_STEP_3;
        }
        else if (animationState == ANIMATION_STEP_3)    // fill in the cooking circle indicator if we were already cooking, otherwise finish animation
        {
            if (!optionTextArea.isFadeAnimationRunning())
            {
                animationState = ANIMATION_STEP_4;
            }
        }
        else if (animationState == ANIMATION_STEP_4)    // start the cooking
        {
            animationState = ANIMATION_DONE;
            isEnterScreenAnimationFinished = true;
        }
    }
}

void CookingView::updateTimer()
{
    if ((minuteTimer != 0) || (secondTimer != 0))        // If the timer is not over
    {
        cookingTimeTextArea.invalidate();
        secondTimer = secondTimer - 1;

        if (secondTimer < 0)    // if the second timer goes below 0, we decrease one minute and go back to 59s
        {
            secondTimer = 59;

            minuteTimer--;

            if (minuteTimer <= CookingView::cookingTimeMinutes - 2)    // Just for demo after 1 min is passed the food is cooked
            {
                isCookingFinished = true;
                minuteTimer = 0;
                secondTimer = 0;
            }

            touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer1, COOKINGTIMETEXTAREABUFFER1_SIZE, "%02d", minuteTimer);
        }

        touchgfx::Unicode::snprintf(cookingTimeTextAreaBuffer2, COOKINGTIMETEXTAREABUFFER2_SIZE, "%02d", secondTimer);

        cookingTimeTextArea.invalidate();
    }
}

void CookingView::changeColorOverviewedMenuBackground()
{
    if (cancelOptionSelected)    // cancel option corresponds to the bottom right corner
    {
        //Change the button image to show that it is selected
        cancelButton.setBitmaps(touchgfx::Bitmap(BITMAP_CANCEL_BUTTON_WHITE_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_CANCEL_BUTTON_WHITE_BACKGROUND_ID));
        cancelButton.invalidate();
    }
    else if (settingMenuSelected)    // setting menu corresponds to the top right corner
    {
        //Change the button image to show that it is selected
        settingsButton.setBitmaps(touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_WHITE_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_WHITE_BACKGROUND_ID));
        settingsButton.invalidate();
    }
    else
    {
        //Change the button image to show that it is selected
        infoButton.setBitmaps(touchgfx::Bitmap(BITMAP_INFO_BUTTON_WHITE_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_INFO_BUTTON_WHITE_BACKGROUND_ID));
        infoButton.invalidate();
    }
}

void CookingView::changeColorUnselectedMenuBackground()
{
    if ((!settingMenuSelected) && (!informationgMenuSelected))  // extra verification to make sure the color is properly reset if we go back to the cooking selection option
    {
        //Change the button images
        settingsButton.setBitmaps(touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_BACKGROUND_ID));
        infoButton.setBitmaps(touchgfx::Bitmap(BITMAP_INFO_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_INFO_BUTTON_BACKGROUND_ID));
        cancelButton.setBitmaps(touchgfx::Bitmap(BITMAP_CANCEL_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_CANCEL_BUTTON_BACKGROUND_ID));
        settingsButton.invalidate();
        infoButton.invalidate();
        cancelButton.invalidate();
    }
    else if (settingMenuSelected)
    {
        infoButton.setBitmaps(touchgfx::Bitmap(BITMAP_INFO_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_INFO_BUTTON_BACKGROUND_ID));
        infoButton.invalidate();
    }
    else
    {
        settingsButton.setBitmaps(touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_BACKGROUND_ID));
        settingsButton.invalidate();
    }
}

void CookingView::updateCircleTimer()
{
    circleEndValue = circleEndValue + Constants::circleIncrementationPerTick;

    if (!isCookingFinished)
    {
        cookingCircle.updateArcEnd(circleEndValue);
    }
}

void CookingView::EXITSCREEN_STATEMACHINE()
{
    if (exitAnimationState > EXIT_ANIMATION_READY)
    {
        if (exitAnimationState == EXIT_ANIMATION_STEP_0)   // reduce the circle to 0
        {
            infoButton.setTouchable(false);
            settingsButton.setTouchable(false);
            cancelButton.setTouchable(false);

            if (circleEndValue > 0) // until we reach 0
            {
                circleEndValue = circleEndValue - circleIndicatorFillerSteps;
                cookingCircle.updateArcEnd(circleEndValue);
            }
            else
            {
                cookingCircle.updateArcEnd(0); // make sure we have the proper ending value
                cookingCircle.setVisible(false);
                cookingCircle.invalidate();
                exitAnimationState = EXIT_ANIMATION_STEP_1;
            }
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_1)   // fade text
        {
            cookingTimeTextArea.startFadeAnimation(0, 15);
            optionTextArea.startFadeAnimation(0, 15);
            exitAnimationState = EXIT_ANIMATION_STEP_2;
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_2)   // fade out image
        {
            if (!cookingTimeTextArea.isFadeAnimationRunning())
            {
                selectedImage.startFadeAnimation(0, 8);
                exitAnimationState = EXIT_ANIMATION_STEP_3;
            }
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_3)   // fade out the relevent buttons
        {
            if (!selectedImage.isFadeAnimationRunning())
            {
                if ((settingMenuSelected) || (informationgMenuSelected))
                {
                    settingsButton.startFadeAnimation(0, 15);
                    infoButton.startFadeAnimation(0, 15);
                }
                timerImage.startFadeAnimation(0, 15);
                cancelButton.startFadeAnimation(0, 15);
                exitAnimationState = EXIT_ANIMATION_STEP_4;
            }
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_4)   // exit screen
        {
            if (!cancelButton.isFadeAnimationRunning())
            {
                if (settingMenuSelected)   // Change screen depending on what option is selected
                {
                    goToSettingMenu();
                }
                else if (informationgMenuSelected)
                {
                    goToInformationMenu();
                }
                else    // both for cancel and end of cooking
                {
                    goToSelectingMenu();
                }
            }
        }
    }
}

void CookingView::clockAnimation()
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

                touchgfx::Unicode::snprintf(clockTextAreaBuffer1, CLOCKTEXTAREABUFFER1_SIZE, "%02d", hourCounter);
            }

            touchgfx::Unicode::snprintf(clockTextAreaBuffer2, CLOCKTEXTAREABUFFER2_SIZE, "%02d", minuteCounter);

            clockTextArea.invalidate();
        }
    }
}

void CookingView::saveSharedData()
{
    saveElementID(elementID);
    saveHourClock(hourCounter);
    saveMinuteClock(minuteCounter);
    savePreviousScreen(Enums::Cooking);
    saveMinuteCookingTimer(minuteTimer);
    saveSecondCookingTimer(secondTimer);
}

/*******************************************************
            Touch screen and simulator related UI interactions
*******************************************************/

void CookingView::pressCancelButtonAction()
{
    isEndingAnimationCalled = true;
    pressCenterKeyFunction();
}

void CookingView::pressSettingButtonAction()
{
    goToSettingMenu();
}

void CookingView::pressInfoButtonAction()
{
    goToInformationMenu();
}
