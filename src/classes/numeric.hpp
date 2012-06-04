#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Numeric
	{
		value_t nonzero(value_t obj);
		value_t coerce(value_t obj, value_t other);
		value_t pos(value_t obj);

		void initialize();
	};
};
