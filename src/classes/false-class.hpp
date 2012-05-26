#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace FalseClass
	{
		value_t to_s();
		value_t rb_xor(value_t value);

		void initialize();
	};
};
