// Events.h
#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>

	/**
	 * @brief 定义事件来源枚举
	 */
	enum class EventSource {
		AXIS,
		BUTTON
	};

	/**
	 * @brief 定义方向枚举
	 */
	enum class Direction {
		CENTER = 0,
		UP,
		DOWN,
		LEFT,
		RIGHT
	};

	/**
	 * @brief 定义事件类型枚举
	 */
	enum class EventType {
		SINGLE_CLICK,
		MULTICLICK,
		LONG_PRESS
	};

	/**
	 * @brief 摇杆事件结构体
	 */
	struct JoystickEvent {
		EventSource	source;     // AXIS 或 BUTTON
		Direction	direction;    // 如果来源是AXIS
		EventType	type;
	};

	/**
	 * @brief 按钮事件结构体
	 */
	struct ButtonEvent {
		EventSource	source;     // BUTTON
		uint8_t		buttonId;       // 按钮ID
		EventType	type;
	};

#endif // EVENTS_H