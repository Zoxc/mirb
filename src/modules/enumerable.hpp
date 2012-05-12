#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Enumerable
	{
		value_t any(value_t obj, value_t block);

		void initialize();
	};
};
