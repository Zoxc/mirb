#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace NilClass
	{
		value_t to_s();
		value_t inspect();
		
		extern value_t class_ref;

		void initialize();
	};
};
