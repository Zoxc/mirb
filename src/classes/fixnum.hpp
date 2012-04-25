#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Fixnum
	{
		value_t to_s(value_t obj);
		value_t times(value_t obj, value_t block);
		value_t add(value_t obj, value_t other);
		value_t sub(value_t obj, value_t other);
		value_t mul(value_t obj, value_t other);
		value_t div(value_t obj, value_t other);
		
		value_t from_size_t(size_t value);
		size_t to_size_t(value_t obj);
		
		value_t from_int(int value);
		int to_int(value_t obj);

		void initialize();
	};
};
