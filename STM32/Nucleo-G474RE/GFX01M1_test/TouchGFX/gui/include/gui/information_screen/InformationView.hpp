#ifndef INFORMATIONVIEW_HPP
#define INFORMATIONVIEW_HPP

#include <gui_generated/information_screen/InformationViewBase.hpp>
#include <gui/information_screen/InformationPresenter.hpp>

class InformationView : public InformationViewBase
{
public:
    InformationView();
    virtual ~InformationView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();


    /*******************************************************
                Handlers and buttons
    *******************************************************/
    virtual void handleTickEvent();

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

#endif // INFORMATIONVIEW_HPP
