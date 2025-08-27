#ifndef SELECTINGMENUPRESENTER_HPP
#define SELECTINGMENUPRESENTER_HPP

#include <gui/model/ModelListener.hpp>
#include <mvp/Presenter.hpp>

using namespace touchgfx;

class SelectingMenuView;

class SelectingMenuPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    SelectingMenuPresenter(SelectingMenuView& v);

    /**
     * The activate function is called automatically when this screen is "switched in"
     * (ie. made active). Initialization logic can be placed here.
     */
    virtual void activate();

    /**
     * The deactivate function is called automatically when this screen is "switched out"
     * (ie. made inactive). Teardown functionality can be placed here.
     */
    virtual void deactivate();

    virtual ~SelectingMenuPresenter() {};

    /*******************************************************
                Passing data through screens
    *******************************************************/
    // Passing the element chosen

    void saveElementID(int16_t savedElementID)
    {
        model->saveElementID(savedElementID);
    }

    // Saving which was the previous screen
    void savePreviousScreen(int16_t savedScreen)
    {
        model->savePreviousScreen(savedScreen);
    }

    int16_t getPreviousScreen()
    {
        return model->getPreviousScreen();
    }

    // Passing the clock settings

    void saveHourClock(int16_t hour)
    {
        model->saveHourClock(hour);
    }

    void saveMinuteClock(int16_t minute)
    {
        model->saveMinuteClock(minute);
    }

    int16_t getHourClock()
    {
        return model->getHourClock();
    }

    int16_t getMinuteClock()
    {
        return model->getMinuteClock();
    }

private:
    SelectingMenuPresenter();

    SelectingMenuView& view;
};

#endif // SELECTINGMENUPRESENTER_HPP
