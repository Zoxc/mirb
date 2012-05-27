#include "nil-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t NilClass::to_s()
	{
		return String::get("");
	}

	value_t NilClass::to_i()
	{
		return Fixnum::from_int(0);
	}

	value_t NilClass::inspect()
	{
		return String::get("nil");
	}
	
	value_t NilClass::nil()
	{
		return value_true;
	}
	
	value_t NilClass::rb_xor(value_t value)
	{
		return Value::test(value) ? value_true : value_false;
	}
	
	void NilClass::initialize()
	{
		method<Arg::Value, &rb_xor>(context->nil_class, "^");
		method<&to_i>(context->nil_class, "to_i");
		method<&to_s>(context->nil_class, "to_s");
		method<&inspect>(context->nil_class, "inspect");
		method<&nil>(context->nil_class, "nil?");

		set_const(context->object_class, Symbol::get("NIL"), value_nil);
	};
};

