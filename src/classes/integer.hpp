#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Integer
	{
		value_t to_i(value_t obj);
		value_t pos(value_t obj);

		void initialize();
	};
};
