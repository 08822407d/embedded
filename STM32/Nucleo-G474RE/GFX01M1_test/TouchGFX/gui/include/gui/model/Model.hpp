#ifndef MODEL_HPP
#define MODEL_HPP

#include <stdint.h>

#include <touchgfx/Utils.hpp>
#include <touchgfx/hal/HAL.hpp>

class ModelListener;

class Model
{
public:
    Model();

    void bind(ModelListener* listener)
    {
        modelListener = listener;
    }

    void tick();


    /*******************************************************
                Passing data through screens
    *******************************************************/
    // Saving which element has been chosen
    void saveElementID(int16_t savedElementID)
    {
        elementID = savedElementID;
    }

    int16_t getElementID()
    {
        return elementID;
    }

    // Saving which was the previous screen
    void savePreviousScreen(int16_t savedScreen)
    {
        previousScreen = savedScreen;
    }

    int16_t getPreviousScreen()
    {
        return previousScreen;
    }

    // Saving the clock settings
    void saveHourClock(int16_t savedHourClock)
    {
        hour = savedHourClock;
    }

    void saveMinuteClock(int16_t savedMinuteClock)
    {
        minute = savedMinuteClock;
    }

    int16_t getHourClock()
    {
        return hour;
    }

    int16_t getMinuteClock()
    {
        return minute;
    }

    // Saving the cooking time remaining
    void saveMinuteCookingTimer(int16_t savedMinuteCookingTimer)
    {
        minuteCookingTimer = savedMinuteCookingTimer;
    }

    void saveSecondCookingTimer(int16_t savedSecondCookingTimer)
    {
        secondCookingTimer = savedSecondCookingTimer;
    }

    int16_t getMinuteCookingTimer()
    {
        return minuteCookingTimer;
    }

    int16_t getSecondCookingTimer()
    {
        return secondCookingTimer;
    }

protected:
    ModelListener* modelListener;
    int16_t elementID, previousScreen, hour, minute, minuteCookingTimer, secondCookingTimer; // initial clock settings arbitrary set to 10:20
};

#endif // MODEL_HPP
