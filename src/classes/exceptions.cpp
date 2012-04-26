#include "exceptions.hpp"
#include "../runtime.hpp"
#include "class.hpp"

namespace Mirb
{
	void initialize_exceptions()
	{
		context->standard_error = define_class(context->object_class, "StandardError", context->exception_class);
		context->name_error = define_class(context->object_class, "NameError", context->standard_error);
		context->type_error = define_class(context->object_class, "TypeError", context->standard_error);
		context->argument_error = define_class(context->object_class, "ArgumentError", context->standard_error);
		context->runtime_error = define_class(context->object_class, "RuntimeError", context->standard_error);
		context->local_jump_error = define_class(context->object_class, "LocalJumpError", context->standard_error);
	}
};

