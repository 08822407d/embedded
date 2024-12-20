// ButtonHandler.tpp
#ifndef BUTTON_HANDLER_TPP
#define BUTTON_HANDLER_TPP

#include "ButtonHandler.hpp"


	ButtonHandler::ButtonHandler(std::shared_ptr<ButtonReader> reader, 
								unsigned long longPressDurationMs, 
								unsigned long multiclickIntervalMs)
		: reader(reader), 
		longPressDurationMs(longPressDurationMs), 
		multiclickIntervalMs(multiclickIntervalMs),
		pollingIntervalMs(JOYSTICK_DEBOUNCE_TIME_MS),
		callback(nullptr),
		buttonCount(reader->getButtonCount()),
		debounceEnabled(true),
		lastUpdateTick(0)
	{
		for(uint8_t i = 0; i < JOYSTICK_MAX_BUTTONS; i++) {
			buttons[i].state = State::IDLE;
			buttons[i].pressStartTick = 0;
			buttons[i].lastClickTick = 0;
			buttons[i].clickCount = 0;
		}
	}

	void ButtonHandler::setLongPressDuration(unsigned long durationMs) {
		this->longPressDurationMs = durationMs;
	}

	void ButtonHandler::setMulticlickInterval(unsigned long intervalMs) {
		this->multiclickIntervalMs = intervalMs;
	}

	void ButtonHandler::setPollingInterval(unsigned long intervalMs) {
		this->pollingIntervalMs = intervalMs;
	}

	void ButtonHandler::attachCallback(ButtonCallback cb) {
		this->callback = cb;
	}

	void ButtonHandler::enableDebounce(bool enable) {
		this->debounceEnabled = enable;
	}

	void ButtonHandler::update() {
		TickType_t currentTick = xTaskGetTickCount();

		// 防抖逻辑
		if (debounceEnabled && (currentTick - lastUpdateTick) < pdMS_TO_TICKS(pollingIntervalMs)) {
			return;
		}
		lastUpdateTick = currentTick;

		uint8_t buttonsState;
		if (!reader->read(buttonsState)) {
			// 读取失败，可添加错误处理
			return;
		}

		for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
			bool pressed = (buttonsState & (1 << i)) != 0;
			handleButtonState(i, pressed, currentTick);
		}
	}

	void ButtonHandler::handleButtonState(uint8_t buttonId, bool pressed, TickType_t currentTick) {
		if(buttonId >= JOYSTICK_MAX_BUTTONS || buttonId >= buttonCount){
			// 无效按键
			return;
		}
		ButtonState &btn = buttons[buttonId];

		TickType_t longPressTicks = pdMS_TO_TICKS(longPressDurationMs);
		TickType_t multiClickTicks = pdMS_TO_TICKS(multiclickIntervalMs);
		
		switch(btn.state) {
			case State::IDLE:
				if (pressed) {
					btn.state = State::PRESSED;
					btn.pressStartTick = currentTick;
				}
				break;
			
			case State::PRESSED:
				if (!pressed) {
					TickType_t pressDuration = currentTick - btn.pressStartTick;
					if (pressDuration >= longPressTicks) {
						// 长按事件
						if (callback) {
							ButtonEvent event;
							event.source = EventSource::BUTTON;
							event.buttonId = buttonId;
							event.type = EventType::LONG_PRESS;
							callback(event);
						}
						btn.state = State::IDLE;
					}
					else {
						// 等待是否为多击
						btn.state = State::WAITING_FOR_MULTICLICK;
						btn.lastClickTick = currentTick;
						btn.clickCount = 1;
					}
				}
				break;
			
			case State::WAITING_FOR_MULTICLICK:
				if (pressed) {
					btn.clickCount++;
					if (btn.clickCount >= 2 && btn.clickCount <= JOYSTICK_MAX_MULTICLICK_COUNT) {
						if (callback) {
							ButtonEvent event;
							event.source = EventSource::BUTTON;
							event.buttonId = buttonId;
							event.type = EventType::MULTICLICK;
							callback(event);
						}
						btn.state = State::PRESSED;
						btn.pressStartTick = currentTick;
					}
					btn.lastClickTick = currentTick;
				}
				else {
					// 检查多击间隔是否超时
					if ((currentTick - btn.lastClickTick) > multiClickTicks) {
						btn.state = State::IDLE;
					}
				}
				break;
			
			case State::LONG_PRESSED:
				if (!pressed) {
					btn.state = State::IDLE;
				}
				break;
		}
	}

#endif // BUTTON_HANDLER_TPP