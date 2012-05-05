#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Comparable
	{
		value_t less(value_t obj, value_t other);
		value_t less_or_equal(value_t obj, value_t other);
		value_t greater(value_t obj, value_t other);
		value_t greater_or_equal(value_t obj, value_t other);
		value_t equal(value_t obj, value_t other);
		value_t between(value_t obj, value_t left, value_t right);

		void initialize();
	};
};
