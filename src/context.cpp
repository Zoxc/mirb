#include "context.hpp"

namespace Mirb
{
	Context *context;

	Context::Context()
	{
		auto start = &object_class;

		while(start != &terminator)
		{
			*start = nullptr;

			start++;
		}
	}
}
