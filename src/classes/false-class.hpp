#pragma once
#include "../value.hpp"

namespace Mirb
{
	namespace FalseClass
	{
		value_t to_s();

		extern value_t class_ref;

		void initialize();
	};
};
