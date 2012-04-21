#include "proc.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Proc::class_ref;
	
	value_t Proc::call(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		auto self = cast<Proc>(obj);

		OnStack<1> os(self);
		
		ProcFrame frame;

		frame.code = self->block;
		frame.obj = self->self;
		frame.name = self->name;
		frame.module = self->module;
		frame.block = block;
		frame.argc = argc;
		frame.argv = argv;
		frame.scopes = self->scopes;

		return call_frame(frame);
	}

	void Proc::initialize()
	{
		Proc::class_ref = define_class(Object::class_ref, "Proc", Object::class_ref);

		static_method<Arg::Self, Arg::Block, Arg::Count, Arg::Values>(Proc::class_ref, "call", &call);
	}
};

