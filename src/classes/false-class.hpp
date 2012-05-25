#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace FalseClass
	{
		value_t to_s();
		value_t xor(value_t value);

		void initialize();
	};
};
