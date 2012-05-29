#include "context.hpp"
#include "collector.hpp"
#include "classes/class.hpp"

namespace Mirb
{
	Context *context;
	LinkedList<ThreadContext> thread_contexts;
	prelude_thread ThreadContext *thread_context;

	Context::Context() :
		bootstrap(true),
		frame(0),
		console_input(0),
		console_output(0),
		console_error(0),
		globals(8)
	{
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
