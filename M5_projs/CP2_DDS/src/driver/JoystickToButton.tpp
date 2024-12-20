#pragma once

// JoystickToButton.tpp
#ifndef JOYSTICK_TO_BUTTON_TPP
#define JOYSTICK_TO_BUTTON_TPP

#include "JoystickToButton.hpp"


	JoystickToButton::JoystickToButton(std::shared_ptr<Joystick> joystick, 
									const std::vector<Direction>& directions,
									unsigned long longPressDurationMs, 
									unsigned long multiclickIntervalMs)
		: joystick(joystick),
		directions(directions),
		longPressDurationMs(longPressDurationMs),
		multiclickIntervalMs(multiclickIntervalMs),
		pollingIntervalMs(JOYSTICK_DEBOUNCE_TIME_MS),
		callback(nullptr),
		directionCount(directions.size()),
		debounceEnabled(true),
		lastUpdateTick(0)
	{
		// 初始化每个方向的虚拟按钮状态
		for(auto &dir : directions){
			VirtualButtonState state = { State::IDLE, 0, 0, 0 };
			buttonStates[dir] = state;
		}

		// 注册摇杆事件回调
		joystick->attachCallback([this](const JoystickEvent& event){
			if(event.source == EventSource::AXIS){
				handleJoystickEvent(event);
			}
		});
	}

	void JoystickToButton::setLongPressDuration(unsigned long durationMs){
		this->longPressDurationMs = durationMs;
	}

	void JoystickToButton::setMulticlickInterval(unsigned long intervalMs){
		this->multiclickIntervalMs = intervalMs;
	}

	void JoystickToButton::setPollingInterval(unsigned long intervalMs){
		this->pollingIntervalMs = intervalMs;
	}

	void JoystickToButton::attachCallback(VirtualButtonCallback cb){
		this->callback = cb;
	}

	void JoystickToButton::enableDebounce(bool enable){
		this->debounceEnabled = enable;
	}

	void JoystickToButton::handleJoystickEvent(const JoystickEvent& event){
		// Check if the direction corresponds to a virtual button
		auto it = buttonStates.find(event.direction);
		if(it == buttonStates.end()){
			return; // Not a direction mapped to a virtual button
		}
		uint8_t buttonId = std::distance(buttonStates.begin(), it);
		VirtualButtonState &btn = it->second;
		TickType_t currentTick = xTaskGetTickCount();

		TickType_t longPressTicks = pdMS_TO_TICKS(longPressDurationMs);
		TickType_t multiClickTicks = pdMS_TO_TICKS(multiclickIntervalMs);

		switch(btn.state){
			case State::IDLE:
				if(event.type == EventType::SINGLE_CLICK){
					btn.state = State::PRESSED;
					btn.pressStartTick = currentTick;
					// Trigger SINGLE_CLICK event
					if(callback){
						ButtonEvent btnEvent = { EventSource::BUTTON, buttonId, EventType::SINGLE_CLICK };
						callback(btnEvent);
					}
				}
				break;

			case State::PRESSED:
				if(event.type == EventType::LONG_PRESS){
					btn.state = State::LONG_PRESSED;
					// Trigger LONG_PRESS event
					if(callback){
						ButtonEvent btnEvent = { EventSource::BUTTON, buttonId, EventType::LONG_PRESS };
						callback(btnEvent);
					}
				}
				break;

			case State::WAITING_FOR_MULTICLICK:
				if(event.type == EventType::MULTICLICK){
					btn.clickCount++;
					if(btn.clickCount >= 2 && btn.clickCount <= JOYSTICK_MAX_MULTICLICK_COUNT){
						if(callback){
							ButtonEvent btnEvent = { EventSource::BUTTON, buttonId, EventType::MULTICLICK };
							callback(btnEvent);
						}
						btn.state = State::PRESSED;
						btn.pressStartTick = currentTick;
					}
					btn.lastClickTick = currentTick;
				}
				else{
					// Check if multiclick interval has passed
					if((currentTick - btn.lastClickTick) > multiClickTicks){
						btn.state = State::IDLE;
					}
				}
				break;

			case State::LONG_PRESSED:
				if(event.direction == Direction::CENTER){
					btn.state = State::IDLE;
				}
				break;
		}
	}

	void JoystickToButton::update(){
		TickType_t currentTick = xTaskGetTickCount();

		// 防抖逻辑
		if(debounceEnabled && (currentTick - lastUpdateTick) < pdMS_TO_TICKS(pollingIntervalMs)){
			return;
		}
		lastUpdateTick = currentTick;

		// Joystick events are handled via callback
		// No need to poll here
	}

#endif // JOYSTICK_TO_BUTTON_TPP