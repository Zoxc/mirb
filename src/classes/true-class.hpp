#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace TrueClass
	{
		value_t to_s();
		
		extern value_t class_ref;

		void initialize();
	};
};
