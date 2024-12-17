// Joystick.tpp
#ifndef _JOYSTICK_TPP_
#define _JOYSTICK_TPP_

#include "Joystick.hpp"

	template <typename T>
	Joystick<T>::Joystick(std::shared_ptr<JoystickReader<T>> reader, 
						unsigned char thresholdPercent, 
						unsigned long longPressDurationMs, 
						unsigned long multiclickIntervalMs)
		: reader(reader), thresholdPercent(thresholdPercent), 
		longPressDurationMs(longPressDurationMs), 
		multiclickIntervalMs(multiclickIntervalMs),
		pollingIntervalMs(JOYSTICK_DEBOUNCE_TIME_MS),
		state(State::IDLE),
		currentDirection(Direction::CENTER),
		pressedDirection(Direction::CENTER),
		pressStartTick(0),
		lastClickTick(0),
		clickCount(0),
		lastUpdateTick(0),
		calibratedCenterX((reader->getMinX() + reader->getMaxX()) / 2),
		calibratedCenterY((reader->getMinY() + reader->getMaxY()) / 2),
		buttonCount(reader->getButtonCount()),
		debounceEnabled(true) // 默认启用防抖
	{
		for(uint8_t i = 0; i < JOYSTICK_MAX_BUTTONS; i++) {
			buttons[i].state = State::IDLE;
			buttons[i].pressStartTick = 0;
			buttons[i].lastClickTick = 0;
			buttons[i].clickCount = 0;
		}
	}

	template <typename T>
	void Joystick<T>::setThresholdPercent(unsigned char thresholdPercent) {
		if (thresholdPercent > 100) thresholdPercent = 100;
		this->thresholdPercent = thresholdPercent;
	}

	template <typename T>
	void Joystick<T>::setLongPressDuration(unsigned long durationMs) {
		this->longPressDurationMs = durationMs;
	}

	template <typename T>
	void Joystick<T>::setMulticlickInterval(unsigned long intervalMs) {
		this->multiclickIntervalMs = intervalMs;
	}

	template <typename T>
	void Joystick<T>::setPollingInterval(unsigned long intervalMs) {
		this->pollingIntervalMs = intervalMs;
	}

	template <typename T>
	void Joystick<T>::attachCallback(JoystickCallback cb) {
		this->callback = cb;
	}

	template <typename T>
	void Joystick<T>::calibrate(int centerX, int centerY) {
		this->calibratedCenterX = centerX;
		this->calibratedCenterY = centerY;
	}

	template <typename T>
	void Joystick<T>::enableDebounce(bool enable) {
		this->debounceEnabled = enable;
	}

	template <typename T>
	void Joystick<T>::update() {
		// 使用xTaskGetTickCount()获取当前时间（ticks）
		TickType_t currentTick = xTaskGetTickCount();
		
		// 如果启用防抖，则检查防抖间隔
		if (debounceEnabled && (currentTick - lastUpdateTick) < pdMS_TO_TICKS(JOYSTICK_DEBOUNCE_TIME_MS)) {
			return;
		}
		lastUpdateTick = currentTick;
		
		int x, y;
		uint8_t buttonsState;
		if (!reader->read(x, y, buttonsState)) {
			// 读取失败，可添加错误处理
			return;
		}
		
		// 处理轴
		Direction detected;
		detectDirection(x, y, detected);
		handleAxisState(detected, currentTick);
		
		// 处理按钮
		for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
			bool pressed = (buttonsState & (1 << i)) != 0;
			handleButtonState(i, pressed, currentTick);
		}
	}

	template <typename T>
	int Joystick<T>::getThresholdValue(int minVal, int maxVal) const {
		int range = maxVal - minVal;
		return (range * thresholdPercent) / 100;
	}

	template <typename T>
	void Joystick<T>::detectDirection(int x, int y, Direction &detected) {
		detected = Direction::CENTER;
		int minX = reader->getMinX();
		int maxX = reader->getMaxX();
		int minY = reader->getMinY();
		int maxY = reader->getMaxY();
		
		int centerX = calibratedCenterX;
		int centerY = calibratedCenterY;
		
		int thresholdX = getThresholdValue(minX, maxX);
		int thresholdY = getThresholdValue(minY, maxY);
		
		if (x > (centerX + thresholdX) && abs(y - centerY) <= thresholdY) {
			detected = Direction::RIGHT;
		}
		else if (x < (centerX - thresholdX) && abs(y - centerY) <= thresholdY) {
			detected = Direction::LEFT;
		}
		else if (y > (centerY + thresholdY) && abs(x - centerX) <= thresholdX) {
			detected = Direction::UP;
		}
		else if (y < (centerY - thresholdY) && abs(x - centerX) <= thresholdX) {
			detected = Direction::DOWN;
		}
		else {
			detected = Direction::CENTER;
		}
	}

	template <typename T>
	void Joystick<T>::handleAxisState(Direction detected, TickType_t currentTick) {
		TickType_t longPressTicks = pdMS_TO_TICKS(longPressDurationMs);
		TickType_t multiClickTicks = pdMS_TO_TICKS(multiclickIntervalMs);

		switch(state) {
			case State::IDLE:
				if (detected != Direction::CENTER) {
					state = State::PRESSED;
					pressedDirection = detected;
					pressStartTick = currentTick;
					if (callback) {
						JoystickEvent event;
						event.source = EventSource::AXIS;
						event.direction = detected;
						event.type = EventType::SINGLE_CLICK;
						callback(event);
					}
				}
				break;
			
			case State::PRESSED:
				if (detected != pressedDirection) {
					if (detected == Direction::CENTER) {
						TickType_t pressDuration = currentTick - pressStartTick;
						if (pressDuration >= longPressTicks) {
							// 已触发长按
							state = State::LONG_PRESSED;
							// 长按事件已在PRESSED状态触发
						}
						else {
							// 等待是否为多击
							state = State::WAITING_FOR_MULTICLICK;
							lastClickTick = currentTick;
							clickCount = 1; // 已有一次点击
						}
					}
					else {
						pressedDirection = detected;
						pressStartTick = currentTick;
						if (callback) {
							JoystickEvent event;
							event.source = EventSource::AXIS;
							event.direction = detected;
							event.type = EventType::SINGLE_CLICK;
							callback(event);
						}
					}
				}
				else {
					// 同一方向按下，检查长按
					TickType_t pressDuration = currentTick - pressStartTick;
					if (pressDuration >= longPressTicks && state != State::LONG_PRESSED) {
						if (callback) {
							JoystickEvent event;
							event.source = EventSource::AXIS;
							event.direction = detected;
							event.type = EventType::LONG_PRESS;
							callback(event);
						}
						state = State::LONG_PRESSED;
					}
				}
				break;
			
			case State::WAITING_FOR_MULTICLICK:
				if (detected == pressedDirection) {
					clickCount++;
					if (clickCount >= 2 && clickCount <= JOYSTICK_MAX_MULTICLICK_COUNT) {
						if (callback) {
							JoystickEvent event;
							event.source = EventSource::AXIS;
							event.direction = detected;
							event.type = EventType::MULTICLICK;
							callback(event);
						}
						state = State::PRESSED;
						pressStartTick = currentTick;
					}
					lastClickTick = currentTick;
				}
				else if (detected != Direction::CENTER) {
					pressedDirection = detected;
					pressStartTick = currentTick;
					if (callback) {
						JoystickEvent event;
						event.source = EventSource::AXIS;
						event.direction = detected;
						event.type = EventType::SINGLE_CLICK;
						callback(event);
					}
					state = State::PRESSED;
				}
				else {
					// 检查多击间隔
					if ((currentTick - lastClickTick) > multiClickTicks) {
						state = State::IDLE;
					}
				}
				break;
			
			case State::LONG_PRESSED:
				if (detected == Direction::CENTER) {
					state = State::IDLE;
				}
				break;
		}
	}

	template <typename T>
	void Joystick<T>::handleButtonState(uint8_t buttonId, bool pressed, TickType_t currentTick) {
		if(buttonId >= JOYSTICK_MAX_BUTTONS || buttonId >= buttonCount){
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
						if (callback) {
							JoystickEvent event;
							event.source = EventSource::BUTTON;
							event.buttonId = buttonId;
							event.type = EventType::LONG_PRESS;
							callback(event);
						}
						btn.state = State::IDLE;
					}
					else {
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
							JoystickEvent event;
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

#endif /* _JOYSTICK_TPP_ */