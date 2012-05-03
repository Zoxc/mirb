#include "proc.hpp"
#include "symbol.hpp"
#include "class.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Proc::call_with_options(value_t self_value, Tuple<Module> *scope, value_t obj, value_t block, size_t argc, value_t argv[])
	{
		auto self = cast<Proc>(obj);

		Frame frame;

		frame.code = self->block;
		frame.obj = self_value;
		frame.name = self->name;
		frame.scope = scope;
		frame.block = block;
		frame.argc = argc;
		frame.argv = argv;
		frame.scopes = self->scopes;

		return call_frame(frame);
	}
	
	value_t Proc::call(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		auto self = cast<Proc>(obj);

		return call_with_options(self->self, self->scope, obj, block, argc, argv);
	}

	void Proc::initialize()
	{
		context->proc_class = define_class("Proc", context->object_class);

		method<Arg::Self, Arg::Block, Arg::Count, Arg::Values>(context->proc_class, "call", &call);
	}
};

