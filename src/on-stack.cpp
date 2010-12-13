#include "on-stack.hpp"

namespace Mirb
{
	size_t *on_stack_reference(value_t &arg)
	{
		return &arg;
	}

	size_t *on_stack_reference(const CharArray &value)
	{
		return 0;
	}
};
