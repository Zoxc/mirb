#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace Fixnum
	{
		extern value_t class_ref;
		
		value_t from_size_t(size_t value);
		size_t to_size_t(value_t obj);
		
		value_t from_int(int value);
		int to_int(value_t obj);

		void initialize();
	};
};
