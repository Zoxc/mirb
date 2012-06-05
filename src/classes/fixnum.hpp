#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Fixnum
	{
		typedef intptr_t int_t;
		
		const intptr_t bits = sizeof(int_t) * 8 - 1;
		const intptr_t width = bits - 1;
		const intptr_t high = INTPTR_MAX >> 1;
		const intptr_t low = INTPTR_MIN >> 1;

		const intptr_t undef = low - 1;

		bool fits(int_t value);
		
		value_t convert(int_t value);

		static_assert(sizeof(int_t) == sizeof(size_t), "int_t must be pointer sized");
		
		value_t zero(int_t obj);
		value_t chr(int_t obj);
		value_t invert(int_t obj);
		value_t rb_size();
		value_t to_s(int_t obj, int_t base);
		value_t to_f(int_t obj);
		value_t times(int_t obj, value_t block);
		value_t upto(int_t obj, int_t arg, value_t block);
		value_t pow(int_t obj, int_t other);
		value_t neg(int_t obj);
		value_t add(int_t obj, value_t other);
		value_t sub(int_t obj, value_t other);
		value_t mul(int_t obj, value_t other);
		value_t div(int_t obj, value_t other);
		value_t mod(int_t obj, value_t other);
		value_t compare(int_t obj, value_t other);
		value_t coerce(int_t obj, value_t other);
		value_t lshift(int_t obj, int_t shift);
		value_t rshift(int_t obj, int_t shift);
		value_t bitwise_and(int_t obj, value_t other);
		value_t bitwise_xor(int_t obj, value_t other);
		value_t bitwise_or(int_t obj, value_t other);

		value_t from_size_t(size_t value);
		size_t to_size_t(value_t obj);
		
		value_t from_int(int_t value);
		int_t to_int(value_t obj);

		void initialize();
	};
};
