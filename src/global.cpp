#include "global.hpp"
#include "classes/symbol.hpp"
#include "runtime.hpp"

namespace Mirb
{
	bool Global::read_only_global(Global *global, Symbol *name, value_t value)
	{
		raise(context->name_error, name->string + " is a read-only variable");
		return false;
	}
};

