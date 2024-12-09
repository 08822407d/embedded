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
		enum BaseType { BINARY = 2, DECIMAL = 10, HEXADECIMAL = 16 };

		Number(T initialValue = 0, T minValue = 0, T maxValue = 10000000, 
			BaseType base = DECIMAL, 
			OverflowBehavior<T>* overflowBehavior = nullptr);
			
		~Number();

		void incrementDigit(int digitIndex);
		void decrementDigit(int digitIndex);
		void setBase(BaseType newBase);
		std::vector<int> getDigits() const;
		std::string toString() const;
		T getValue() const { return currentValue; }
		void setOverflowBehavior(OverflowBehavior<T>* newBehavior);

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