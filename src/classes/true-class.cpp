#include "true-class.hpp"
#include "object.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t TrueClass::to_s()
	{
		return String::get("true");
	}
	
	value_t TrueClass::rb_xor(value_t value)
	{
		return value->test() ? value_false : value_true;
	}
	
	void TrueClass::initialize()
	{
		method<Value, &rb_xor>(context->true_class, "^");
		method<&to_s>(context->true_class, "to_s");

		set_const(context->object_class, Symbol::get("TRUE"), value_true);
	};
};

