#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Integer
	{
		value_t to_i(value_t obj);
		value_t ord(value_t obj);

		void initialize();
	};
};
