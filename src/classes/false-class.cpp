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
	
	value_t FalseClass::xor(value_t value)
	{
		return Value::test(value) ? value_true : value_false;
	}
	
	void FalseClass::initialize()
	{
		method<Arg::Value>(context->false_class, "^", &xor);
		method(context->false_class, "to_s", &to_s);

		set_const(context->object_class, Symbol::get("FALSE"), value_false);
	};
};

