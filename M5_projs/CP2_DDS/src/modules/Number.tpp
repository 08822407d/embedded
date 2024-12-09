#ifndef _NUMBER_TPP_
#define _NUMBER_TPP_

#include "OverflowBehavior.hpp"


	template <typename T>
	Number<T>::Number(T initialValue, T minValue, T maxValue, 
					BaseType baseType, OverflowBehavior<T>* overflowBehaviorStrategy)
		: currentValue(initialValue), minValue(minValue), maxValue(maxValue), base(baseType),
		overflowBehavior(overflowBehaviorStrategy) {
		if (!overflowBehavior) {
			// 默认使用ClampOverflowBehavior
			overflowBehavior.reset(new ClampOverflowBehavior<T>());
		}
		applyOverflow();
	}

	template <typename T>
	Number<T>::~Number() {
		// unique_ptr 会自动释放资源
	}

	template <typename T>
	void Number<T>::incrementDigit(int digitIndex) {
		T placeValue = static_cast<T>(pow(base, digitIndex));
		currentValue += placeValue;
		applyOverflow();
	}

	template <typename T>
	void Number<T>::decrementDigit(int digitIndex) {
		T placeValue = static_cast<T>(pow(base, digitIndex));
		currentValue -= placeValue;
		applyOverflow();
	}

	template <typename T>
	void Number<T>::setBase(BaseType newBase) {
		base = newBase;
		// 可选：根据新进制验证currentValue是否仍在合法范围
	}

	template <typename T>
	std::vector<int> Number<T>::getDigits() const {
		std::vector<int> digits;
		T value = currentValue;
		if (value == 0) {
			digits.push_back(0);
			return digits;
		}
		while (value > 0) {
			digits.push_back(static_cast<int>(value % base));
			value /= base;
		}
		// 反转为从高位到低位
		std::reverse(digits.begin(), digits.end());
		return digits;
	}

	template <typename T>
	std::string Number<T>::toString() const {
		std::string str = "";
		T value = currentValue;
		if (value == 0) return "0";
		while (value > 0) {
			int digit = static_cast<int>(value % base);
			if (base == HEXADECIMAL && digit >= 10) {
				str += ('A' + (digit - 10));
			} else {
				str += ('0' + digit);
			}
			value /= base;
		}
		std::reverse(str.begin(), str.end());
		return str;
	}

	template <typename T>
	void Number<T>::applyOverflow() {
		currentValue = overflowBehavior->handleOverflow(currentValue, minValue, maxValue);
	}

	template <typename T>
	void Number<T>::setOverflowBehavior(OverflowBehavior<T>* newBehavior) {
		if (newBehavior) {
			overflowBehavior.reset(newBehavior);
			applyOverflow();
		}
	}

#endif /* _NUMBER_TPP_ */