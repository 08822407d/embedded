#include <gui/information_screen/InformationView.hpp>

#include <gui/common/Enums.hpp>
#include <texts/TextKeysAndLanguages.hpp>

InformationView::InformationView():
    tickCounter(0),
    transitionCounter(0),
    clockCounter(0),    // Used for the clock tick count
    secondCounter(0),   // Initial clock arbitrary set to 10:20 in model.c , overwritten by the settings if any
    minuteCounter(0),
    hourCounter(0),
    previousScreen(0),
    minuteTimer(0),
    secondTimer(0)
{

}

void InformationView::setupScreen()
{
    readSharedData();

    InformationViewBase::setupScreen();
}

void InformationView::tearDownScreen()
{
    InformationViewBase::tearDownScreen();

    saveSharedData();
}


/*******************************************************
            Handlers and buttons
*******************************************************/

void InformationView::handleTickEvent()
{
    tickCounter++;
    transitionCounter++;

    // Animations

    if (previousScreen == Enums::Cooking)
    {
        if (tickCounter >= 60)  // every 60 ticks = 1s
        {
            tickCounter = 0;

            updateTimer();
        }
    }

    if (transitionCounter >= 900) // 900 ticks is 15s
    {
        goToScreenInfo2();
    }

    clockAnimation();
}



/*******************************************************
            UI animations and interactions
*******************************************************/

void InformationView::updateTimer() // only used if we were in the cooking screen before to keep the timer running
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
}

void InformationView::clockAnimation()
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

            }
        }
    }
}

void InformationView::readSharedData()
{
    previousScreen = getPreviousScreen();
    hourCounter = getHourClock();
    minuteCounter = getMinuteClock();
    minuteTimer = getMinuteCookingTimer();
    secondTimer = getSecondCookingTimer();
}

void InformationView::saveSharedData()
{
    // Saving the passing elements
    saveHourClock(hourCounter);
    saveMinuteClock(minuteCounter);
    saveMinuteCookingTimer(minuteTimer);
    saveSecondCookingTimer(secondTimer);
}
