// JoystickConfig.h
#ifndef JOYSTICK_CONFIG_H
#define JOYSTICK_CONFIG_H

	// 防抖时间（毫秒）
	#define JOYSTICK_DEBOUNCE_TIME_MS 50

	// 多击最大间隔时间（毫秒）
	#define JOYSTICK_MULTICLICK_INTERVAL_MS 500

	// 长按持续时间（毫秒）
	#define JOYSTICK_LONG_PRESS_DURATION_MS 1000

	// 多击的最大点击次数
	#define JOYSTICK_MAX_MULTICLICK_COUNT 5

	// 默认最大按钮数量（用户可在项目中定义以覆盖默认值）
	#ifndef JOYSTICK_MAX_BUTTONS
	#  define JOYSTICK_MAX_BUTTONS 5
	#endif

#endif // JOYSTICK_CONFIG_H