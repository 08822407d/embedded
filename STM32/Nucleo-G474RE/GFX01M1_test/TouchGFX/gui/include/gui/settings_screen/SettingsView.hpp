#ifndef SETTINGSVIEW_HPP
#define SETTINGSVIEW_HPP

#include <gui_generated/settings_screen/SettingsViewBase.hpp>
#include <gui/settings_screen/SettingsPresenter.hpp>

class SettingsView : public SettingsViewBase
{
public:
    enum EnterAnimationState
    {
        ANIMATION_READY = 0,
        ANIMATION_STEP_0,   // enter screen animation: title fade in
        ANIMATION_STEP_1,   // option backgrounds fade in
        ANIMATION_STEP_2,   // text option fade in
        ANIMATION_DONE
    };

    enum ExitAnimationState
    {
        EXIT_ANIMATION_READY = 0,
        EXIT_ANIMATION_STEP_0,   // option text fade out
        EXIT_ANIMATION_STEP_1,   // option backgrounds fade out
        EXIT_ANIMATION_STEP_2,   // title fade out
        EXIT_ANIMATION_STEP_3,   // exit screen
        EXIT_ANIMATION_DONE
    };

    enum HighlightedItem
    {
        BACK_BUTTON,
        LANGUAGE_BUTTON,
        CLOCK_BUTTON,
        LANGAUGE_SELECTION,
        HOUR_SETTING,
        MINUTE_SETTING
    };

    SettingsView();
    virtual ~SettingsView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();

    /*******************************************************
                Handlers and hardware buttons
    *******************************************************/
    virtual void handleTickEvent();

    virtual void pressUpKeyFunction();
    virtual void pressDownKeyFunction();
    virtual void pressCenterKeyFunction();

    virtual void languageButtonClicked();
    virtual void clockButtonClicked();
    virtual void hourIsClicked();
    virtual void minuteIsClicked();
    virtual void returnButtonIsClicked();

    /*******************************************************
                Passing data through screens
    *******************************************************/

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



protected:
    int16_t tickCounter, clockCounter, secondCounter, minuteCounter, hourCounter;
    int16_t selectedOption, numberOptions, selectedLanguage, numberLanguage;
    int16_t previousScreen, minuteTimer, secondTimer;
    bool isEnterScreenAnimationFinished, isEndingAnimationCalled;
    EnterAnimationState animationState;
    ExitAnimationState exitAnimationState;
    HighlightedItem highlightedItem;

private:
    /*******************************************************
            UI animations and interactions
    *******************************************************/
    void customEntrySetup();
    void setUpTheClock();

    void ENTERSCREEN_STATEMACHINE();
    void EXITSCREEN_STATEMACHINE();
    void updateTimer();
    void clockAnimation();

    void increaseClockSettings();
    void decreaseClockSettings();
    void languageSettingsOption();

    void readSharedData();
    void saveSharedData();
};

#endif // SETTINGSVIEW_HPP
