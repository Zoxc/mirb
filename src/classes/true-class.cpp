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
		return Value::test(value) ? value_false : value_true;
	}
	
	void TrueClass::initialize()
	{
		method<Arg::Value>(context->true_class, "^", &rb_xor);
		method(context->true_class, "to_s", &to_s);

		set_const(context->object_class, Symbol::get("TRUE"), value_true);
	};
};

