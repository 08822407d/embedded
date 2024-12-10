// Joystick.tpp
#ifndef _JOYSTICK_TPP_
#define _JOYSTICK_TPP_

#include "Joystick.hpp"

	template <typename T>
	Joystick<T>::Joystick(std::shared_ptr<JoystickReader<T>> reader, 
						unsigned char thresholdPercent, 
						unsigned long longPressDuration, 
						unsigned long multiclickInterval)
		: reader(reader), thresholdPercent(thresholdPercent), 
		longPressDuration(longPressDuration), 
		multiclickInterval(multiclickInterval),
		pollingInterval(JOYSTICK_DEBOUNCE_TIME_MS),
		state(State::IDLE),
		currentDirection(Direction::CENTER),
		pressedDirection(Direction::CENTER),
		pressStartTime(0),
		lastClickTime(0),
		clickCount(0),
		lastUpdateTime(0),
		calibratedCenterX((reader->getMinX() + reader->getMaxX()) / 2),
		calibratedCenterY((reader->getMinY() + reader->getMaxY()) / 2),
		buttonCount(reader->getButtonCount())
	{
		for(uint8_t i = 0; i < JOYSTICK_MAX_BUTTONS; i++) {
			buttons[i].state = State::IDLE;
			buttons[i].pressStartTime = 0;
			buttons[i].lastClickTime = 0;
			buttons[i].clickCount = 0;
		}
	}

	template <typename T>
	void Joystick<T>::setThresholdPercent(unsigned char thresholdPercent) {
		if (thresholdPercent > 100) thresholdPercent = 100;
		this->thresholdPercent = thresholdPercent;
	}

	template <typename T>
	void Joystick<T>::setLongPressDuration(unsigned long duration) {
		this->longPressDuration = duration;
	}

	template <typename T>
	void Joystick<T>::setMulticlickInterval(unsigned long interval) {
		this->multiclickInterval = interval;
	}

	template <typename T>
	void Joystick<T>::setPollingInterval(unsigned long interval) {
		this->pollingInterval = interval;
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
	void Joystick<T>::update(TickType_t currentMillis) {
		// 防抖逻辑：仅在防抖时间后更新
		if ((currentMillis - lastUpdateTime) < JOYSTICK_DEBOUNCE_TIME_MS) {
			return;
		}
		lastUpdateTime = currentMillis;
		
		int x, y;
		uint8_t buttonsState;
		if (!reader->read(x, y, buttonsState)) {
			// 读取失败，可添加错误处理
			return;
		}
		
		// 处理轴
		Direction detected;
		detectDirection(x, y, detected);
		handleAxisState(detected, currentMillis);
		
		// 处理按钮
		for(uint8_t i = 0; i < buttonCount && i < JOYSTICK_MAX_BUTTONS; i++) {
			bool pressed = (buttonsState & (1 << i)) != 0;
			handleButtonState(i, pressed, currentMillis);
		}
	}

	template <typename T>
	int Joystick<T>::getThresholdValue(int minVal, int maxVal) const {
		int range = maxVal - minVal;
		// 使用整数运算计算阈值
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
	void Joystick<T>::handleAxisState(Direction detected, unsigned long currentMillis) {
		switch(state) {
			case State::IDLE:
				if (detected != Direction::CENTER) {
					// 刚按下方向
					state = State::PRESSED;
					pressedDirection = detected;
					pressStartTime = currentMillis;
					
					// 触发单击事件
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
						// 松开
						unsigned long pressDuration = currentMillis - pressStartTime;
						if (pressDuration >= longPressDuration) {
							// 已触发长按
							state = State::LONG_PRESSED;
							// 长按事件已在 PRESSED 状态中触发
						}
						else {
							// 等待是否为多击
							state = State::WAITING_FOR_MULTICLICK;
							lastClickTime = currentMillis;
							clickCount = 1; // 已有一次点击
						}
					}
					else {
						// 改变方向，视为新的单击
						pressedDirection = detected;
						pressStartTime = currentMillis;
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
					// 仍在同一方向按下，检查是否触发长按
					unsigned long pressDuration = currentMillis - pressStartTime;
					if (pressDuration >= longPressDuration && state != State::LONG_PRESSED) {
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
					// 多击
					clickCount++;
					if (clickCount >= 2 && clickCount <= JOYSTICK_MAX_MULTICLICK_COUNT) { // 判断是否达到多击次数
						if (callback) {
							JoystickEvent event;
							event.source = EventSource::AXIS;
							event.direction = detected;
							event.type = EventType::MULTICLICK;
							callback(event);
						}
						state = State::PRESSED;
						pressStartTime = currentMillis;
					}
					lastClickTime = currentMillis;
				}
				else if (detected != Direction::CENTER) {
					// 另一方向按下，视为新的单击
					pressedDirection = detected;
					pressStartTime = currentMillis;
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
					// 检查多击间隔是否超时
					if ((currentMillis - lastClickTime) > multiclickInterval) {
						// 多击超时，返回IDLE
						state = State::IDLE;
					}
				}
				break;
			
			case State::LONG_PRESSED:
				if (detected == Direction::CENTER) {
					// 松开长按
					state = State::IDLE;
				}
				break;
		}
	}

	template <typename T>
	void Joystick<T>::handleButtonState(uint8_t buttonId, bool pressed, unsigned long currentMillis) {
		if(buttonId >= JOYSTICK_MAX_BUTTONS || buttonId >= buttonCount){
			// 无效按键
			return;
		}
		ButtonState &btn = buttons[buttonId];
		
		switch(btn.state) {
			case State::IDLE:
				if (pressed) {
					// 按下
					btn.state = State::PRESSED;
					btn.pressStartTime = currentMillis;
				}
				break;
			
			case State::PRESSED:
				if (!pressed) {
					// 松开
					unsigned long pressDuration = currentMillis - btn.pressStartTime;
					if (pressDuration >= longPressDuration) {
						// 已触发长按
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
						// 等待是否为多击
						btn.state = State::WAITING_FOR_MULTICLICK;
						btn.lastClickTime = currentMillis;
						btn.clickCount = 1; // 已有一次点击
					}
				}
				break;
			
			case State::WAITING_FOR_MULTICLICK:
				if (pressed) {
					// 多击
					btn.clickCount++;
					if (btn.clickCount >= 2 && btn.clickCount <= JOYSTICK_MAX_MULTICLICK_COUNT) { // 判断是否达到多击次数
						if (callback) {
							JoystickEvent event;
							event.source = EventSource::BUTTON;
							event.buttonId = buttonId;
							event.type = EventType::MULTICLICK;
							callback(event);
						}
						btn.state = State::PRESSED;
						btn.pressStartTime = currentMillis;
					}
					btn.lastClickTime = currentMillis;
				}
				else {
					// 松开
					// 检查多击间隔是否超时
					if ((currentMillis - btn.lastClickTime) > multiclickInterval) {
						// 多击超时，返回IDLE
						btn.state = State::IDLE;
					}
				}
				break;
			
			case State::LONG_PRESSED:
				if (!pressed) {
					// 松开长按
					btn.state = State::IDLE;
				}
				break;
		}
	}

#endif /* _JOYSTICK_TPP_ */