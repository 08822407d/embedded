#ifndef _NUMBER_TPP_
#define _NUMBER_TPP_

#include "OverflowBehavior.hpp"


	template <typename T>
	Number<T>::Number(T initialValue, T minValue,
			T maxValue, BaseType baseType,
			OverflowBehavior<T>* overflowBehaviorStrategy)
		: currentValue(initialValue), minValue(minValue),
			maxValue(maxValue), base(baseType),
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


	// 增加指定位的值（等同于 addDigit(digitIndex, 1)）
	template <typename T>
	void Number<T>::incrementDigit(int digitIndex) {
		addDigit(digitIndex, static_cast<T>(1));
	}

	// 减少指定位的值（等同于 subDigit(digitIndex, 1)）
	template <typename T>
	void Number<T>::decrementDigit(int digitIndex) {
		subDigit(digitIndex, static_cast<T>(1));
	}

	// 为指定位数的数字增加任意值
	template <typename T>
	void Number<T>::addDigit(int digitIndex, T value) {
		T placeValue = static_cast<T>(std::pow(base, digitIndex));
		this->currentValue += value * placeValue;
		applyOverflow();
	}

	// 为指定位数的数字减少任意值
	template <typename T>
	void Number<T>::subDigit(int digitIndex, T value) {
		T placeValue = static_cast<T>(std::pow(base, digitIndex));
		this->currentValue -= value * placeValue;
		applyOverflow();
	}

	// 设置进制
	template <typename T>
	void Number<T>::setBase(BaseType newBase) {
		this->base = newBase;
		// 可选：根据新进制验证currentValue是否仍在合法范围
		// 例如，重新计算digits或调整currentValue
	}

	// 获取各个位的值（从高位到低位）
	template <typename T>
	std::vector<int> Number<T>::getDigits() const {
		std::vector<int> digits;
		T value = this->currentValue;
		if (value == 0) {
			digits.push_back(0);
			return digits;
		}
		while (value > 0) {
			digits.push_back(static_cast<int>(value % this->base));
			value /= this->base;
		}
		// 反转为从高位到低位
		std::reverse(digits.begin(), digits.end());
		return digits;
	}

	// 将当前数值转换为字符串表示
	template <typename T>
	std::string Number<T>::toString() const {
		std::string str = "";
		T value = this->currentValue;
		if (value == 0) return "0";
		while (value > 0) {
			int digit = static_cast<int>(value % this->base);
			if (this->base == HEXADECIMAL && digit >= 10) {
				str += ('A' + (digit - 10));
			} else {
				str += ('0' + digit);
			}
			value /= this->base;
		}
		std::reverse(str.begin(), str.end());
		return str;
	}

	// 获取当前值
	// getValue() 已在头文件中实现

	// 设置新的溢出行为
	template <typename T>
	void Number<T>::setOverflowBehavior(OverflowBehavior<T>* newBehavior) {
		if (newBehavior) {
			this->overflowBehavior.reset(newBehavior);
			applyOverflow();
		}
	}

	// 应用溢出处理
	template <typename T>
	void Number<T>::applyOverflow() {
		this->currentValue = this->overflowBehavior->handleOverflow(
			this->currentValue, this->minValue, this->maxValue
		);
	}

	// 四则运算操作符

	// 加法
	template <typename T>
	Number<T> Number<T>::operator+(const Number<T>& other) const {
		Number<T> result(*this);
		result.currentValue += other.currentValue;
		result.applyOverflow();
		return result;
	}

	// 减法
	template <typename T>
	Number<T> Number<T>::operator-(const Number<T>& other) const {
		Number<T> result(*this);
		result.currentValue -= other.currentValue;
		result.applyOverflow();
		return result;
	}

	// 乘法
	template <typename T>
	Number<T> Number<T>::operator*(const Number<T>& other) const {
		Number<T> result(*this);
		result.currentValue *= other.currentValue;
		result.applyOverflow();
		return result;
	}

	// 除法
	template <typename T>
	Number<T> Number<T>::operator/(const Number<T>& other) const {
		Number<T> result(*this);
		if (other.currentValue != 0) {
			result.currentValue /= other.currentValue;
		} else {
			// 处理除以零的情况，根据需求决定
			// 这里选择不修改 currentValue
		}
		result.applyOverflow();
		return result;
	}

	// 复合赋值操作符

	// 加法赋值
	template <typename T>
	Number<T>& Number<T>::operator+=(const Number<T>& other) {
		this->currentValue += other.currentValue;
		applyOverflow();
		return *this;
	}

	// 减法赋值
	template <typename T>
	Number<T>& Number<T>::operator-=(const Number<T>& other) {
		this->currentValue -= other.currentValue;
		applyOverflow();
		return *this;
	}

	// 乘法赋值
	template <typename T>
	Number<T>& Number<T>::operator*=(const Number<T>& other) {
		this->currentValue *= other.currentValue;
		applyOverflow();
		return *this;
	}

	// 除法赋值
	template <typename T>
	Number<T>& Number<T>::operator/=(const Number<T>& other) {
		if (other.currentValue != 0) {
			this->currentValue /= other.currentValue;
		} else {
			// 处理除以零的情况，根据需求决定
			// 这里选择不修改 currentValue
		}
		applyOverflow();
		return *this;
	}

#endif /* _NUMBER_TPP_ */