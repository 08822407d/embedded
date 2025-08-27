#ifndef COOKINGVIEW_HPP
#define COOKINGVIEW_HPP

#include <gui_generated/cooking_screen/CookingViewBase.hpp>
#include <gui/cooking_screen/CookingPresenter.hpp>

class CookingView : public CookingViewBase
{
public:
    enum EnterAnimationState
    {
        ANIMATION_READY = 0,
        ANIMATION_STEP_0,   // enter screen animation: timer and cancel button fade in
        ANIMATION_STEP_1,   // cooking background appears
        ANIMATION_STEP_2,   // option text and timer fade in
        ANIMATION_STEP_3,   // fill in the cooking circle indicator
        ANIMATION_STEP_4,   // start the cooking
        ANIMATION_DONE
    };

    enum ExitAnimationState
    {
        EXIT_ANIMATION_READY = 0,
        EXIT_ANIMATION_STEP_0,   // reduce the cooking circle indicator to 0
        EXIT_ANIMATION_STEP_1,   // fade out text
        EXIT_ANIMATION_STEP_2,   // fade out selected image
        EXIT_ANIMATION_STEP_3,   // fade out buttons
        EXIT_ANIMATION_STEP_4,   // exit screen
        EXIT_ANIMATION_DONE
    };

    CookingView();
    virtual ~CookingView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /*******************************************************
                Handlers and hardware buttons
    *******************************************************/
    virtual void handleTickEvent();

    virtual void pressCenterKeyFunction();
    virtual void pressUpKeyFunction();
    virtual void pressDownKeyFunction();
    virtual void pressRightKeyFunction();
    virtual void pressLeftKeyFunction();

    /*******************************************************
                Passing data through screens
    *******************************************************/
    virtual void saveElementID(int16_t savedElementID)
    {
        presenter->saveElementID(savedElementID);
    }

    int16_t getElementID()
    {
        return presenter->getElementID();
    }

    // Saving which was the previous screen
    void savePreviousScreen(int16_t savedScreen)
    {
        presenter->savePreviousScreen(savedScreen);
    }

    int16_t getPreviousScreen()
    {
        return presenter->getPreviousScreen();
    }
    // Passing the clock settings

    void saveHourClock(int16_t savedHour)
    {
        presenter->saveHourClock(savedHour);
    }

    void saveMinuteClock(int16_t savedMinute)
    {
        presenter->saveMinuteClock(savedMinute);
    }

    int16_t getHourClock()
    {
        return presenter->getHourClock();
    }

    int16_t getMinuteClock()
    {
        return presenter->getMinuteClock();
    }

    // Saving the cooking time remaining

    void saveMinuteCookingTimer(int16_t savedMinuteCookingTimer)
    {
        presenter->saveMinuteCookingTimer(savedMinuteCookingTimer);
    }

    void saveSecondCookingTimer(int16_t savedSecondCookingTimer)
    {
        presenter->saveSecondCookingTimer(savedSecondCookingTimer);
    }

    int16_t getMinuteCookingTimer()
    {
        return presenter->getMinuteCookingTimer();
    }

    int16_t getSecondCookingTimer()
    {
        return presenter->getSecondCookingTimer();
    }

    /*******************************************************
                Touch screen and simulator related actions
    *******************************************************/
    virtual void pressCancelButtonAction();
    virtual void pressSettingButtonAction();
    virtual void pressInfoButtonAction();

protected:
    int16_t elementID, minuteTimer, secondTimer;
    static int16_t cookingTimeMinutes;
    float circleEndValue, circleIndicatorFillerSteps;
    int16_t tickCounter, clockCounter, secondCounter, minuteCounter, hourCounter;
    bool settingMenuSelected, informationgMenuSelected, cancelOptionSelected, isCookingFinished, isEnterScreenAnimationFinished, isEndingAnimationCalled;
    EnterAnimationState animationState;
    ExitAnimationState exitAnimationState;
    bool hasCenterKeyBeenPressed;

private:
    /*******************************************************
            UI animations and interactions
    *******************************************************/
    void setUpTheClock();
    void customSetupScreen();
    void setUpInfoAboutCookingFood();

    void ENTERSCREEN_STATEMACHINE();
    void EXITSCREEN_STATEMACHINE();
    void updateCircleTimer();
    void updateTimer();
    void clockAnimation();

    void changeColorOverviewedMenuBackground();
    void changeColorUnselectedMenuBackground();
    void saveSharedData();
};

#endif // COOKINGVIEW_HPP
