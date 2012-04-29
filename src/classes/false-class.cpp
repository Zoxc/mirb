#include "false-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t FalseClass::to_s()
	{
		return String::from_literal("false");
	}

	void FalseClass::initialize()
	{
		method(context->false_class, "to_s", &to_s);

		set_const(context->object_class, Symbol::from_literal("FALSE"), value_false);
	};
};

