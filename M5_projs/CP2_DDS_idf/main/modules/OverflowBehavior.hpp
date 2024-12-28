#pragma once

#ifndef _OVERFLOW_BEHAVIOR_HPP_
#define _OVERFLOW_BEHAVIOR_HPP_


	template <typename T>
	class OverflowBehavior {
	public:
		virtual ~OverflowBehavior() {}
		virtual T handleOverflow(T value, T min, T max) = 0;
	};

	template <typename T>
	class ClampOverflowBehavior : public OverflowBehavior<T> {
	public:
		virtual T handleOverflow(T value, T min, T max) override {
			if (value < min) return min;
			if (value > max) return max;
			return value;
		}
	};

	template <typename T>
	class WrapOverflowBehavior : public OverflowBehavior<T> {
	public:
		virtual T handleOverflow(T value, T min, T max) override {
		T range_size = max - min + 1;
		// 计算相对位置并进行模运算
		T relative = (value - min) % range_size;
		if (relative < 0) {
			relative += range_size;
		}
		// 计算包装后的值
		T wrappedValue = min + relative;

		return wrappedValue;
		}
	};

#endif /* _OVERFLOW_BEHAVIOR_HPP_ */