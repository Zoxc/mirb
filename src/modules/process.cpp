#include "process.hpp"
#include "../classes/fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Process::pid()
	{
		return Fixnum::from_int(1);
	}

	void Process::initialize()
	{
		context->process_module = define_module("Process");

		singleton_method<>(context->process_module, "pid", &pid);
	}
};
