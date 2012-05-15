#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace NilClass
	{
		value_t to_s();
		value_t inspect();
		value_t nil();
		
		void initialize();
	};
};
