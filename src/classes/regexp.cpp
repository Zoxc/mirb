#include "regexp.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	void Regexp::initialize()
	{
		context->regexp_class = define_class("Regexp", context->object_class);
	}
};

