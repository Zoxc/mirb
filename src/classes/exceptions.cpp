#include "exceptions.hpp"
#include "../runtime.hpp"
#include "class.hpp"
#include "string.hpp"

namespace Mirb
{
	SystemStackError::SystemStackError() : Exception(context->system_stack_error, String::get("Stack oveflow"), Mirb::backtrace())
	{
	}

	void initialize_exceptions()
	{
		context->standard_error = define_class("StandardError", context->exception_class);

		context->system_stack_error = define_class("SystemStackError", context->exception_class);

		context->name_error = define_class("NameError", context->standard_error);
		context->type_error = define_class("TypeError", context->standard_error);
		context->syntax_error = define_class("SyntaxError", context->standard_error);
		context->argument_error = define_class("ArgumentError", context->standard_error);
		context->runtime_error = define_class("RuntimeError", context->standard_error);
		context->local_jump_error = define_class("LocalJumpError", context->standard_error);
		context->load_error = define_class("LoadError", context->standard_error);
		context->system_call_error = define_class("SystemCallError", context->standard_error);
		
		context->signal_exception = define_class("SignalException", context->exception_class);
		context->interrupt_class = define_class("Interrupt", context->signal_exception);
	}
};

