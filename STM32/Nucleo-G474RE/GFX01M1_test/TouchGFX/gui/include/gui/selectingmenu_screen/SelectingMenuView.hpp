#ifndef SELECTINGMENUVIEW_HPP
#define SELECTINGMENUVIEW_HPP

#include <gui_generated/selectingmenu_screen/SelectingMenuViewBase.hpp>
#include <gui/selectingmenu_screen/SelectingMenuPresenter.hpp>

class SelectingMenuView : public SelectingMenuViewBase
{
public:
    enum EnterAnimationState
    {
        ANIMATION_READY = 0,
        ANIMATION_STEP_0,   // enter screen animation: info and settings button fade in
        ANIMATION_STEP_1,   // fade in selected image
        ANIMATION_STEP_2,   // fade in right/left buttons and clock
        ANIMATION_STEP_3,   // scroll up the bottom menu
        ANIMATION_STEP_4,   // fade in the text within the bottom menu
        ANIMATION_DONE
    };

    enum ExitAnimationState
    {
        EXIT_ANIMATION_READY = 0,
        EXIT_ANIMATION_STEP_0,   // set the START text white and fade out buttons
        EXIT_ANIMATION_STEP_1,   // fade out text
        EXIT_ANIMATION_STEP_2,   // scroll down the bottom menu
        EXIT_ANIMATION_STEP_3,   // exit screen
        EXIT_ANIMATION_DONE
    };

    enum ChangeItemState
    {
        ITEM_CHANGE_ANIMATION_READY = 0,
        ITEM_CHANGE_ANIMATION_STEP_0,   // start the button movement animation
        ITEM_CHANGE_ANIMATION_STEP_1,   // fade out the image and the option text
        ITEM_CHANGE_ANIMATION_STEP_2,   // fade out the image and the option text
        ITEM_CHANGE_ANIMATION_STEP_3,   // change the image and the text to the new element
        ITEM_CHANGE_ANIMATION_STEP_4,   // fade in the image and the option text
        ITEM_CHANGE_ANIMATION_STEP_5,   // fade in the image and the option text
        ITEM_CHANGE_ANIMATION_STEP_6,   // move back the button to its original position
        ITEM_CHANGE_ANIMATION_DONE
    };

    SelectingMenuView();
    virtual ~SelectingMenuView() {}
    virtual void setupScreen();
    virtual void tearDownScreen();


    /*******************************************************
                Handlers and hardware buttons
    *******************************************************/
    virtual void handleTickEvent();

    virtual void hardwareCenterKeyAction();
    virtual void hardwareRightKeyAction();
    virtual void hardwareLeftKeyAction();
    virtual void hardwareUpKeyAction();
    virtual void hardwareDownKeyAction();

    /*******************************************************
                Passing data through screens
    *******************************************************/
    // Passing the element chosen

    virtual void saveElementID(int16_t savedElementID)
    {
        presenter->saveElementID(savedElementID);
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


    /*******************************************************
                Touch screen and simulator related actions
    *******************************************************/
    virtual void pressRightButtonAction();
    virtual void pressLeftButtonAction();
    virtual void pressSettingButtonAction();
    virtual void pressInfoButtonAction();
    virtual void hideRemainingElements();

protected:
    int16_t elementID, numberOptions;
    int16_t tickCounter, clockCounter, secondCounter, minuteCounter, hourCounter, animationTickCounter;
    bool settingMenuSelected, informationgMenuSelected;
    bool isEnterScreenAnimationFinished, isBottomContainerAnimationFinished, isButtonAnimationCalled, isEndingAnimationCalled, isButtonRightSelected;
    EnterAnimationState animationState;
    ExitAnimationState exitAnimationState;
    ChangeItemState changeItemAnimationState;

private:
    /*******************************************************
            UI animations and interactions
    *******************************************************/
    void setUpEnterScreen();
    void setUpTheClock();
    void changeSelectedItem();

    void ENTERSCREEN_STATEMACHINE();
    void screenEnterAnimation();
    void fadeInRemainingElements();
    void fadeInTexts();
    void ANIMATION_CHANGE_STATEMACHINE();
    void EXITSCREEN_STATEMACHINE();
    void clockAnimation();

    void changeColorOverviewedMenuBackground();
    void changeColorUnselectedMenuBackground();

    void saveSharedData();
};

#endif // SELECTINGMENUVIEW_HPP
