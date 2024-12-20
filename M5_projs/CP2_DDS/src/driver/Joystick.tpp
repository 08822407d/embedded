// Joystick.tpp
#ifndef JOYSTICK_TPP
#define JOYSTICK_TPP

#include "Joystick.hpp"


	Joystick::Joystick(std::shared_ptr<JoystickReader> reader, 
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
		calibratedCenterY((reader->getMinY() + reader->getMaxY()) / 2)
	{
		// 初始化状态
	}

	void Joystick::setThresholdPercent(unsigned char thresholdPercent) {
		if (thresholdPercent > 100) thresholdPercent = 100;
		this->thresholdPercent = thresholdPercent;
	}

	void Joystick::setLongPressDuration(unsigned long durationMs) {
		this->longPressDurationMs = durationMs;
	}

	void Joystick::setMulticlickInterval(unsigned long intervalMs) {
		this->multiclickIntervalMs = intervalMs;
	}

	void Joystick::setPollingInterval(unsigned long intervalMs) {
		this->pollingIntervalMs = intervalMs;
	}

	void Joystick::attachCallback(std::function<void(const JoystickEvent&)> cb) {
		this->callback = cb;
	}

	void Joystick::calibrate(int centerX, int centerY) {
		this->calibratedCenterX = centerX;
		this->calibratedCenterY = centerY;
	}

	void Joystick::enableDebounce(bool enable) {
		// 防抖启用与否通过设置轮询间隔
		if (enable) {
			this->pollingIntervalMs = JOYSTICK_DEBOUNCE_TIME_MS;
		}
		else {
			this->pollingIntervalMs = 0; // 设置为0表示不进行防抖
		}
	}

	void Joystick::update() {
		TickType_t currentTick = xTaskGetTickCount();

		// 防抖逻辑
		if (pollingIntervalMs > 0 && (currentTick - lastUpdateTick) < pdMS_TO_TICKS(pollingIntervalMs)) {
			return;
		}
		lastUpdateTick = currentTick;

		int x, y;
		if (!reader->read(x, y)) {
			// 读取失败，可添加错误处理
			return;
		}

		// 处理轴
		Direction detected;
		detectDirection(x, y, detected);
		handleAxisState(detected, currentTick);
	}

	int Joystick::getThresholdValue(int minVal, int maxVal) const {
		int range = maxVal - minVal;
		return (range * thresholdPercent) / 100;
	}

	void Joystick::detectDirection(int x, int y, Direction &detected) {
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

	void Joystick::handleAxisState(Direction detected, TickType_t currentTick) {
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
							// 长按事件已在 PRESSED 状态中触发
						}
						else {
							// 等待是否为多击
							state = State::WAITING_FOR_MULTICLICK;
							lastClickTick = currentTick;
							clickCount = 1;
						}
					}
					else {
						// 改变方向，视为新的单击
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
					// 多击
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
					// 另一方向按下，视为新的单击
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
					// 检查多击间隔是否超时
					if ((currentTick - lastClickTick) > multiClickTicks) {
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

#endif // JOYSTICK_TPP