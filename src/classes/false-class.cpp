#include "false-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t FalseClass::to_s()
	{
		return String::get("false");
	}
	
	value_t FalseClass::rb_xor(value_t value)
	{
		return Value::test(value) ? value_true : value_false;
	}
	
	void FalseClass::initialize()
	{
		method<Arg::Value, &rb_xor>(context->false_class, "^");
		method<&to_s>(context->false_class, "to_s");

		set_const(context->object_class, Symbol::get("FALSE"), value_false);
	};
};

