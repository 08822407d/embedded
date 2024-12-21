#pragma once

#include <Wire.h>


#define JOY_ADDR	0x38


// Joystick数据结构
typedef struct JoystickData {
	int8_t	x;
	int8_t	y;
	int8_t	center;
} JoyStickData_s;


// extern Joystick joystick;

extern void readJoyStick(TwoWire *wire, JoyStickData_s *data);
extern void readJoyStick_16bit(TwoWire *wire, JoyStickData_s *data);
extern void initJoystick(void);



#ifndef _M5_JOYSTICKHAT_MODULE_HPP_
#define _M5_JOYSTICKHAT_MODULE_HPP_

#include <Wire.h>
#include <functional>
#include <map>

#include "driver/IModule.hpp"


	// 事件枚举
	enum M5JoystickHATEventType {
		EVT_BTN_MIDDLE_CLICK,
		EVT_BTN_MIDDLE_DOUBLECLICK,
		EVT_BTN_MIDDLE_LONGPRESS,
		// EVT_BTN_TOP_CLICK,
		// EVT_BTN_TOP_DOUBLECLICK,
		// EVT_BTN_TOP_LONGPRESS,
		EVT_DIR_UP_CLICK,
		EVT_DIR_UP_DOUBLECLICK,
		EVT_DIR_UP_LONGPRESS,
		EVT_DIR_DOWN_CLICK,
		EVT_DIR_DOWN_DOUBLECLICK,
		EVT_DIR_DOWN_LONGPRESS,
		EVT_DIR_LEFT_CLICK,
		EVT_DIR_LEFT_DOUBLECLICK,
		EVT_DIR_LEFT_LONGPRESS,
		EVT_DIR_RIGHT_CLICK,
		EVT_DIR_RIGHT_DOUBLECLICK,
		EVT_DIR_RIGHT_LONGPRESS,
		// ... etc
	};

	// 回调类型
	using M5JoystickHATCallback = std::function<void()>;

	// 方向定义
	enum JoyDirection {
		DIR_UP = 0,
		DIR_DOWN,
		DIR_LEFT,
		DIR_RIGHT
	};

	class M5JoystickHAT : public IModule {
	public:
		M5JoystickHAT(uint8_t i2cAddr);
		virtual ~M5JoystickHAT() {}

		bool update() override;
		const char* getName() const override { return "M5JoystickHAT Module"; }

		// 注册回调
		void attachCallback(M5JoystickHATEventType eventType, M5JoystickHATCallback cb);

		// 可选: 提供 setLongPressTime, setDoubleClickInterval, setDirectionThreshold 等

	private:
		uint8_t address;

		// 存储硬件读到的数据
		int xVal, yVal, midBtn;
		bool midBtnPressed;

		// 事件判定相关
		unsigned long lastUpdateMs;

		// 每个事件对应一个回调
		std::map<M5JoystickHATEventType, M5JoystickHATCallback> callbacks;

		// (示例) 按钮状态机/方向状态机
		// 这里为了示例，简化仅处理中键 & top键 & 4向
		// 真实情况可拆分/封装更复杂的事件机

		// 读取硬件
		bool readFromHardware();

		// 处理按钮(中键/顶部)的事件判定
		void handleButtons(unsigned long now);

		// 处理摇杆方向(4向)事件判定
		void handleDirections(unsigned long now);

		// 一个简单阈值判断
		bool isDirectionPressed(JoyDirection dir);

		// 触发事件
		void triggerEvent(M5JoystickHATEventType et);

		// 其他成员(防抖、多击、长按计时等)...
	};

#endif /* _M5_JOYSTICKHAT_MODULE_HPP_ */