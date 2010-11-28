#include "proc.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"
#include "../arch/support.hpp"

namespace Mirb
{
	value_t Proc::class_ref;

	mirb_compiled_block(proc_call)
	{
		auto self = cast<Proc>(obj);

		return Arch::Support::closure_call(self->block->compiled, self->scopes, 0, Mirb::value_nil, self->self, block, argc, argv);
	}

	void Proc::initialize()
	{
		Proc::class_ref = define_class(Object::class_ref, "Proc", Object::class_ref);

		define_method(Proc::class_ref, "call", proc_call);
	}
};

