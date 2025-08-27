#include <gui/selectingmenu_screen/SelectingMenuView.hpp>

#include <gui/common/Constants.hpp>
#include <gui/common/Enums.hpp>
#include <images/BitmapDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Color.hpp>

#include <touchgfx/Utils.hpp>


SelectingMenuView::SelectingMenuView():
    elementID(0), // Set the initial selected element
    numberOptions(4),   // Set the amount of cooking options according to the CookingOptions enum in Enums.hpp
    tickCounter(0),     // Used for the animations
    clockCounter(0),    // Used for the clock tick count
    secondCounter(0),   // Initial clock arbitrary set to 10:20 in model.c , overwritten by the settings if any
    minuteCounter(0),
    hourCounter(0),
    animationTickCounter(0),
    settingMenuSelected(false),
    informationgMenuSelected(false),
    isEnterScreenAnimationFinished(false),
    isBottomContainerAnimationFinished(false),
    isButtonAnimationCalled(false),
    isEndingAnimationCalled(false),
    isButtonRightSelected(false),
    animationState(ANIMATION_STEP_0),
    exitAnimationState(EXIT_ANIMATION_STEP_0),
    changeItemAnimationState(ITEM_CHANGE_ANIMATION_READY)
{

}

void SelectingMenuView::setupScreen()
{
    SelectingMenuViewBase::setupScreen();

    setUpEnterScreen();
    screenEnterAnimation();
}

void SelectingMenuView::tearDownScreen()
{
    SelectingMenuViewBase::tearDownScreen();

    saveSharedData();
}



/*******************************************************
            Handlers and hardware buttons
*******************************************************/

void SelectingMenuView::handleTickEvent()
{
    tickCounter++;

    // Animations

    if (!isEnterScreenAnimationFinished)
    {
        ENTERSCREEN_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    if (isButtonAnimationCalled)
    {
        ANIMATION_CHANGE_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    if (isEndingAnimationCalled)
    {
        EXITSCREEN_STATEMACHINE(); // The screen entering animation is divided in multiple steps for performance improvements
    }

    // Clock

    clockAnimation();
}

void SelectingMenuView::hardwareCenterKeyAction()
{
    if ((!isEnterScreenAnimationFinished) || (!isEndingAnimationCalled))  // wait for the enter animation to be finished
    {
        isEndingAnimationCalled = true;  // all center key options lead to leaving the screen
    }
}

void SelectingMenuView::hardwareRightKeyAction()
{
    if ((!isEnterScreenAnimationFinished) || (!isEndingAnimationCalled))
    {
        if (settingMenuSelected)       // If we are not in the cooking selection option, the right and left keys will be used to navigaate to the information and setting options
        {
            informationgMenuSelected = true;
            settingMenuSelected = false;
            changeColorOverviewedMenuBackground();
            changeColorUnselectedMenuBackground();
        }
        else if (informationgMenuSelected)
        {
            informationgMenuSelected = false;
            settingMenuSelected = true;
            changeColorOverviewedMenuBackground();
            changeColorUnselectedMenuBackground();
        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_READY)
        {
            elementID = elementID + 1;
            isButtonRightSelected = true;
            changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_0;
            isButtonAnimationCalled = true;
        }
    }
}

void SelectingMenuView::hardwareLeftKeyAction()
{
    if ((!isEnterScreenAnimationFinished) || (!isEndingAnimationCalled))  // wait for the enter animation to be finished
    {
        if (settingMenuSelected)        // If we are not in the cooking selection option, the right and left keys will be used to navigaate to the information and setting options
        {
            informationgMenuSelected = true;
            settingMenuSelected = false;
            changeColorOverviewedMenuBackground();
            changeColorUnselectedMenuBackground();
        }
        else if (informationgMenuSelected)
        {
            informationgMenuSelected = false;
            settingMenuSelected = true;
            changeColorOverviewedMenuBackground();
            changeColorUnselectedMenuBackground();
        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_READY)
        {
            elementID = elementID - 1;
            isButtonRightSelected = false;
            changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_0;
            isButtonAnimationCalled = true;
        }
    }
}

void SelectingMenuView::hardwareUpKeyAction()
{
    if ((isEnterScreenAnimationFinished) || (isEndingAnimationCalled))  // wait for the enter animation to be finished
    {
        if ((!settingMenuSelected) || (informationgMenuSelected))      // No need to update the setting background image again if it the settings option is already selected
        {
            informationgMenuSelected = true;    // the first selected option will always be the information menu

            changeColorOverviewedMenuBackground();
        }
    }
}

void SelectingMenuView::hardwareDownKeyAction()
{
    if ((isEnterScreenAnimationFinished) || (isEndingAnimationCalled))  // wait for the enter animation to be finished
    {
        if ((settingMenuSelected) || (informationgMenuSelected))      // If we are in the upper menu we go back to the cooking selection
        {
            settingMenuSelected = false;
            informationgMenuSelected = false;

            changeColorUnselectedMenuBackground();
        }
    }
}

/*******************************************************
            UI animations and interactions
*******************************************************/

void SelectingMenuView::setUpEnterScreen()
{
    setUpTheClock();

    infoButton.setTouchable(false);
    settingsButton.setTouchable(false);

    changeSelectedItem();   // Initialize the correct selected item to be shown

    SelectingMenuView::invalidate();
}

void SelectingMenuView::setUpTheClock()
{
    hourCounter = getHourClock();
    touchgfx::Unicode::snprintf(clockTextAreaBuffer1, CLOCKTEXTAREABUFFER1_SIZE, "%02d", hourCounter);

    minuteCounter = getMinuteClock();
    touchgfx::Unicode::snprintf(clockTextAreaBuffer2, CLOCKTEXTAREABUFFER2_SIZE, "%02d", minuteCounter);
}

void SelectingMenuView::ENTERSCREEN_STATEMACHINE()
{
    if (animationState > ANIMATION_READY)
    {

        if (animationState == ANIMATION_STEP_0)
        {

            if (!settingsButton.isFadeAnimationRunning())   // We wait for the setting and info buttons to finish fading in before moving to the next step
            {
                selectedImage.startFadeAnimation(255, 15);
                animationState = ANIMATION_STEP_1;
            }
        }
        else if (animationState == ANIMATION_STEP_1)    // fade in the selected image
        {
            if (!selectedImage.isFadeAnimationRunning())    // We wait for the selected element image to finish fading in before moving to the next step
            {
                fadeInRemainingElements();
                animationState = ANIMATION_STEP_2;
            }
        }
        else if (animationState == ANIMATION_STEP_2)    // fade in the remaining elements (buttons + clock)
        {
            if (!leftButton.isFadeAnimationRunning())   // We wait for the longuest animation to finish before moving to the next step
            {
                start_option_background.startMoveAnimation(Constants::X_bottomContainer, Constants::Y_ending_bottomContainer, Constants::duration_moveAnimation_bottomContainer);
                animationState = ANIMATION_STEP_3;
            }
        }
        else if (animationState == ANIMATION_STEP_3)    // move bottom container up
        {
            if (!start_option_background.isMoveAnimationRunning())   // We wait for the moving up animation to finish before moving to the next step
            {
                fadeInTexts();
                animationState = ANIMATION_STEP_4;
            }
        }
        else if (animationState == ANIMATION_STEP_4)    // fade in the remaining texts
        {
            if (!startTextArea.isFadeAnimationRunning())   // We wait for the longuest animation to finish before moving to the next step
            {
                infoButton.setTouchable(true);
                settingsButton.setTouchable(true);
                animationState = ANIMATION_DONE;
                isEnterScreenAnimationFinished = true;

            }
        }
    }
}

void SelectingMenuView::screenEnterAnimation()
{
    clockTextArea.startFadeAnimation(255, 15);
}

void SelectingMenuView::fadeInRemainingElements()
{
    leftButton.startFadeAnimation(255, 30);
    rightButton.startFadeAnimation(255, 30);
}

void SelectingMenuView::fadeInTexts()
{
    optionTextArea.startFadeAnimation(255, 15);
    startTextArea.startFadeAnimation(255, 40);
}

void SelectingMenuView::changeSelectedItem()
{
    if (elementID < 0)  // Make sure we alternate between teh right options
    {
        elementID = numberOptions - 1;
    }
    else if (elementID > numberOptions - 1)
    {
        elementID = 0;
    }

    switch (elementID)      // Change the image, name and cooking time for the selected element
    {
    case Enums::Rice:       // Rice = 0
        selectedImage.setBitmap(BITMAP_RICE_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_WHITERICE));
        break;
    case Enums::Pancake:
        selectedImage.setBitmap(BITMAP_PANCAKES_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_PANCAKES));
        break;
    case Enums::Dumpling:
        selectedImage.setBitmap(BITMAP_ICON_DUMPLINGS_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_DUMPLINGS));
        break;
    case Enums::Fish:
        selectedImage.setBitmap(BITMAP_FISH_3D_ID);
        optionTextArea.setTypedText(touchgfx::TypedText(T_FISH));
        break;
    }

    selectedImage.invalidate();
    optionTextArea.invalidate();

}

void SelectingMenuView::changeColorOverviewedMenuBackground()
{
    if (settingMenuSelected)    // setting menu corresponds to the top right corner
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

void SelectingMenuView::changeColorUnselectedMenuBackground()
{
    if ((!settingMenuSelected) && (!informationgMenuSelected))  // extra verification to make sure the color is properly reset if we go back to the cooking selection option
    {
        //Change the button images
        settingsButton.setBitmaps(touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_SETTINGS_BUTTON_BACKGROUND_ID));
        infoButton.setBitmaps(touchgfx::Bitmap(BITMAP_INFO_BUTTON_BACKGROUND_ID), touchgfx::Bitmap(BITMAP_INFO_BUTTON_BACKGROUND_ID));
        settingsButton.invalidate();
        infoButton.invalidate();
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

void SelectingMenuView::ANIMATION_CHANGE_STATEMACHINE()
{
    if (changeItemAnimationState > ITEM_CHANGE_ANIMATION_READY)
    {
        if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_0) // start the button movement animation
        {

            if (isButtonRightSelected)
            {
                if (rightButton.getX() != Constants::X_ending_rightButton)
                {
                    rightButton.moveRelative(1, 0);
                }
                else
                {
                    optionTextArea.startFadeAnimation(0, 15);
                    changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_1;
                }
            }
            else
            {
                if (leftButton.getX() != Constants::X_ending_leftButton)
                {
                    leftButton.moveRelative(-1, 0);
                }
                else
                {
                    optionTextArea.startFadeAnimation(0, 15);
                    changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_1;
                }
            }

        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_1)    // fade out the option text
        {
            if (optionTextArea.getAlpha() == 0)
            {
                optionTextArea.setVisible(false);
                optionTextArea.invalidate();
                selectedImage.startFadeAnimation(0, 5);
                changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_2;
            }

        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_2)    // fade out the image
        {
            if (!selectedImage.isFadeAnimationRunning())    // We wait for the selected element image to finish fading in before moving to the next step
            {
                changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_3;
            }

        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_3)    // change the image and the text to the new
        {
            changeSelectedItem();
            selectedImage.startFadeAnimation(255, 5);
            changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_4;
        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_4)    // fade in the image
        {
            if (!selectedImage.isFadeAnimationRunning())
            {
                optionTextArea.setVisible(true);
                optionTextArea.invalidate();
                optionTextArea.startFadeAnimation(255, 15);
                changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_5;
            }
        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_5)    // fade in the text
        {
            if (optionTextArea.getAlpha() == 255)
            {
                changeItemAnimationState = ITEM_CHANGE_ANIMATION_STEP_6;
            }
        }
        else if (changeItemAnimationState == ITEM_CHANGE_ANIMATION_STEP_6)    // move back the button to its original position
        {
            if (isButtonRightSelected)
            {
                if (rightButton.getX() != Constants::X_starting_righButton)
                {
                    rightButton.moveRelative(-1, 0);
                }
                else
                {
                    changeItemAnimationState = ITEM_CHANGE_ANIMATION_READY; // Ready for next animation
                    isButtonAnimationCalled = false;
                }
            }
            else
            {
                if (leftButton.getX() != Constants::X_starting_leftButton)
                {
                    leftButton.moveRelative(1, 0);
                }
                else
                {
                    changeItemAnimationState = ITEM_CHANGE_ANIMATION_READY; // Ready for next animation
                    isButtonAnimationCalled = false;
                }
            }
        }
    }
}

void SelectingMenuView::EXITSCREEN_STATEMACHINE()
{
    if (exitAnimationState > EXIT_ANIMATION_READY)
    {
        if (exitAnimationState == EXIT_ANIMATION_STEP_0)   // set the START text white and fade out buttons
        {
            if ((!settingMenuSelected) && (!informationgMenuSelected)) // If going to the cooking screen we turn the start button white
            {
                startTextArea.setColor(touchgfx::Color::getColorFromRGB(255, 255, 255));
                startTextArea.invalidate();
            }
            rightButton.startFadeAnimation(0, 15);
            leftButton.startFadeAnimation(0, 15);
            exitAnimationState = EXIT_ANIMATION_STEP_1;

        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_1)   // fade out text
        {
            if (!rightButton.isFadeAnimationRunning())    // We wait for the selected button to finish fading in before moving to the next step
            {
                startTextArea.startFadeAnimation(0, 10);
                optionTextArea.startFadeAnimation(0, 10);
                exitAnimationState = EXIT_ANIMATION_STEP_2;
            }
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_2)   // scroll down the bottom menu
        {
            if (!startTextArea.isFadeAnimationRunning())    // We wait for the selected button to finish fading in before moving to the next step
            {
                start_option_background.startMoveAnimation(Constants::X_bottomContainer, Constants::Y_starting_bottomContainer, Constants::duration_moveAnimation_bottomContainer);
                exitAnimationState = EXIT_ANIMATION_STEP_3;
            }
        }
        else if (exitAnimationState == EXIT_ANIMATION_STEP_3)   // exit screen
        {
            if (!start_option_background.isMoveAnimationRunning())   // We wait for the moving up animation to finish before exiting the screen
            {
                if (settingMenuSelected)   // Change screen depending on what option is selected
                {
                    goToSettingScreen();
                }
                else if (informationgMenuSelected)
                {
                    goToInformationScreen();
                }
                else
                {
                    goToCookingScreen();
                }
                exitAnimationState = EXIT_ANIMATION_READY;
            }

        }
    }
}

void SelectingMenuView::clockAnimation()
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

void SelectingMenuView::hideRemainingElements()
{
    // Called when Information Button or Settings Button is clicked
    clockTextArea.startFadeAnimation(0, 15);
    selectedImage.startFadeAnimation(0, 15);
    infoButton.startFadeAnimation(0, 30);
    settingsButton.startFadeAnimation(0, 30);
}

void SelectingMenuView::saveSharedData()
{
    saveElementID(elementID);
    saveHourClock(hourCounter);
    saveMinuteClock(minuteCounter);
    savePreviousScreen(Enums::Settings);
}
/*******************************************************
            Touch screen and simulator related UI interactions
*******************************************************/

void SelectingMenuView::pressRightButtonAction()
{
    hardwareRightKeyAction();
}

void SelectingMenuView::pressLeftButtonAction()
{
    hardwareLeftKeyAction();
}

void SelectingMenuView::pressSettingButtonAction()
{
    settingMenuSelected = true;
    hardwareCenterKeyAction();
}

void SelectingMenuView::pressInfoButtonAction()
{
    informationgMenuSelected = true;
    hardwareCenterKeyAction();
}
