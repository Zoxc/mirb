#include "proc.hpp"
#include "symbol.hpp"
#include "class.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Proc::call(Proc *self, value_t block, size_t argc, value_t argv[])
	{
		return call_code(self->block, self->self, self->name, self->scope, self->scopes, block, argc, argv);
	}
	
	value_t Proc::rb_new(value_t block)
	{
		return get_proc(block);
	}

	void Proc::initialize()
	{
		context->proc_class = define_class("Proc", context->object_class);
		
		singleton_method<Arg::Block, &rb_new>(context->proc_class, "new");

		method<Self<Proc>, Arg::Block, Arg::Count, Arg::Values, &call>(context->proc_class, "call");
		method<Self<Proc>, Arg::Block, Arg::Count, Arg::Values, &call>(context->proc_class, "[]");
	}
};

