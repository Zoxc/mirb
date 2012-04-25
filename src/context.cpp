#include "context.hpp"

namespace Mirb
{
	Context *context;

	Context::Context()
	{
		mark([&](value_t &entry) {
			entry = value_nil;
		});
	}
}
