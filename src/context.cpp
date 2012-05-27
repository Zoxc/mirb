#include "context.hpp"
#include "collector.hpp"
#include "classes/class.hpp"

namespace Mirb
{
	Context *context;

	Context::Context() :
		bootstrap(true),
		exception_frame_origin(0),
		frame(0),
		globals(8)
	{
		mirb_debug(can_throw = false);

		auto start = &object_class;

		while(start != &terminator)
		{
			*start = nullptr;

			start++;
		}
	}

	void Context::setup()
	{
		object_scope = Tuple<Module>::allocate(1);
		(*object_scope)[0] = object_class;
	}
}
