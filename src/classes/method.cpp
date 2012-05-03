#include "proc.hpp"
#include "symbol.hpp"
#include "class.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	void Method::initialize()
	{
		context->method_class = define_class(context->object_class, "Method", context->object_class);
	}
};

