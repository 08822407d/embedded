#ifndef INFORMATIONDEMOVIEW_HPP
#define INFORMATIONDEMOVIEW_HPP

#include <gui_generated/informationdemo_screen/InformationDemoViewBase.hpp>
#include <gui/informationdemo_screen/InformationDemoPresenter.hpp>

class InformationDemoView : public InformationDemoViewBase
{
public:
    InformationDemoView();
    virtual ~InformationDemoView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();
    /*******************************************************
                Handlers and buttons
    *******************************************************/
    virtual void handleTickEvent();

    virtual void pressCenterKeyFunction();

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

    int16_t tickCounter, transitionCounter, clockCounter, secondCounter, minuteCounter, hourCounter;
    int16_t previousScreen, minuteTimer, secondTimer;

private:
    /*******************************************************
                UI animations and interactions
    *******************************************************/
    void updateTimer();
    void clockAnimation();

    void readSharedData();
    void saveSharedData();
};

#endif // INFORMATIONDEMOVIEW_HPP
