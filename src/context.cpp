#include "context.hpp"
#include "collector.hpp"

namespace Mirb
{
	Context *context;

	Context::Context() : globals(0)
	{
		auto start = &object_class;

		while(start != &terminator)
		{
			*start = nullptr;

			start++;
		}
	}
}
