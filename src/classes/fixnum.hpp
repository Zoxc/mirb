#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Fixnum
	{
		typedef intptr_t int_t;

		static_assert(sizeof(int_t) == sizeof(size_t), "int_t must be pointer sized");
		
		value_t zero(int_t obj);
		value_t to_s(int_t obj);
		value_t to_f(int_t obj);
		value_t times(int_t obj, value_t block);
		value_t upto(int_t obj, int_t arg, value_t block);
		value_t neg(int_t obj);
		value_t add(int_t obj, value_t other);
		value_t sub(int_t obj, value_t other);
		value_t mul(int_t obj, value_t other);
		value_t div(int_t obj, value_t other);
		value_t mod(int_t obj, value_t other);
		value_t compare(int_t obj, value_t other);

		value_t from_size_t(size_t value);
		size_t to_size_t(value_t obj);
		
		value_t from_int(int_t value);
		int_t to_int(value_t obj);

		void initialize();
	};
};
