#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Fixnum
	{
		typedef intptr_t int_t;

		static_assert(sizeof(int_t) == sizeof(size_t), "int_t must be pointer sized");
		
		value_t zero(value_t obj);
		value_t to_s(value_t obj);
		value_t times(value_t obj, value_t block);
		value_t upto(value_t obj, value_t arg, value_t block);
		value_t pos(value_t obj);
		value_t neg(value_t obj);
		value_t add(value_t obj, value_t other);
		value_t sub(value_t obj, value_t other);
		value_t mul(value_t obj, value_t other);
		value_t div(value_t obj, value_t other);
		value_t compare(value_t obj, value_t other);

		value_t from_size_t(size_t value);
		size_t to_size_t(value_t obj);
		
		value_t from_int(int_t value);
		int_t to_int(value_t obj);

		void initialize();
	};
};
