#pragma once

#ifndef _NUMBER_HPP_
#define _NUMBER_HPP_

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include "OverflowBehavior.hpp"



	template <typename T>
	class Number {
		static_assert(std::is_arithmetic<T>::value, "Number requires a numeric type.");

	public:
		enum BaseType {
			BINARY = 2,
			DECIMAL = 10,
			HEXADECIMAL = 16
		};

		Number(T initialValue = 0, T minValue = std::numeric_limits<T>::min(),
			T maxValue = std::numeric_limits<T>::max(), BaseType base = DECIMAL, 
			OverflowBehavior<T>* overflowBehavior = nullptr);
			
		~Number();

		// 增加指定位的值（等同于 addDigit(digitIndex, 1)）
		void incrementDigit(int digitIndex);
		// 减少指定位的值（等同于 subDigit(digitIndex, 1)）
		void decrementDigit(int digitIndex);
		// 为指定位数的数字增加任意值
		void addDigit(int digitIndex, T value);
		// 为指定位数的数字减少任意值
		void subDigit(int digitIndex, T value);
		// 设置进制
		void setBase(BaseType newBase);
		// 获取当前进制
		BaseType setBase() const { return base; }

		// 获取指定位的数值
		int getDigit(int digitIndex) const;
		// 设置指定位的数值
		void setDigit(int digitIndex, int value);
		// 获取各个位的值（从高位到低位）
		std::vector<int> getDigits() const;
		// 将当前数值转换为字符串表示
		std::string toString() const;
		// 获取当前值
		T getValue() const { return currentValue; }
		// 重设当前值
		void setValue(T newValue);
		// 设置新的溢出行为
		void setOverflowBehavior(OverflowBehavior<T>* newBehavior);

		// 四则运算操作符
		Number<T> operator+(const Number<T>& other) const;
		Number<T> operator-(const Number<T>& other) const;
		Number<T> operator*(const Number<T>& other) const;
		Number<T> operator/(const Number<T>& other) const;

		// 复合赋值操作符
		Number<T>& operator+=(const Number<T>& other);
		Number<T>& operator-=(const Number<T>& other);
		Number<T>& operator*=(const Number<T>& other);
		Number<T>& operator/=(const Number<T>& other);

	private:
		T currentValue;
		T minValue;
		T maxValue;
		BaseType base;
		std::unique_ptr<OverflowBehavior<T>> overflowBehavior;

		void applyOverflow();
	};

#include "Number.tpp" // 包含模板类的实现

#endif /* NUMBER */