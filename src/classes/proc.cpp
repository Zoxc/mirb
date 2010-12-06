#include "proc.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"
#include "../arch/support.hpp"

namespace Mirb
{
	value_t Proc::class_ref;
	
	value_t Proc::call(value_t obj, value_t block, size_t argc, value_t argv[])
	{
		auto self = cast<Proc>(obj);

		return Arch::Support::closure_call(self->block->compiled, self->scopes, self->self, 0, Mirb::value_nil, block, argc, argv);
	}

	void Proc::initialize()
	{
		Proc::class_ref = define_class(Object::class_ref, "Proc", Object::class_ref);

		static_method<Arg::Self, Arg::Block, Arg::Count, Arg::Values>(Proc::class_ref, "call", &call);
	}
};

