#include "context.hpp"

namespace Mirb
{
	Context *context;

	Context::Context()
	{
		mark_fields([&](Class *&entry) {
			entry = nullptr;
		});
	}
}
