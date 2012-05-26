#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace NilClass
	{
		value_t to_s();
		value_t to_i();
		value_t inspect();
		value_t nil();
		value_t rb_xor(value_t value);
		
		void initialize();
	};
};
