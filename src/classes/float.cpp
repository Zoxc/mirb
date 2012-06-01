#include "float.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "fixnum.hpp"

namespace Mirb
{
	value_t Float::to_s(Float *obj)
	{
		char_t buffer[32];

		size_t length = sprintf((char *)buffer, "%f", obj->value);

		return CharArray(buffer, length).to_string();
	}
	
	value_t Float::to_f(value_t obj)
	{
		return obj;
	}
	
	value_t Float::zero(Float *obj)
	{
		return Value::from_bool(obj->value == 0.0);
	}
	
	template<typename F, size_t string_length> value_t coerce_op(Float *obj, value_t other, const char (&string)[string_length], F func)
	{
		Float *right = try_cast<Float>(other);

		if(prelude_likely(right != 0))
			return func(right->value);
		else
			return coerce(obj, Symbol::get(string), other);
	}
	
	value_t Float::add(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "+", [&](double right) { return Collector::allocate<Float>(obj->value + right); });
	}
	
	value_t Float::sub(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "-", [&](double right) { return Collector::allocate<Float>(obj->value - right); });
	}
	
	value_t Float::mul(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "*", [&](double right) { return Collector::allocate<Float>(obj->value * right); });
	}
	
	value_t Float::div(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "/", [&](double right) {
			if(prelude_unlikely(right == 0.0))
				zero_division_error();

			return Collector::allocate<Float>(obj->value / right);
		});
	}

	value_t Float::compare(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "+", [&](double right) { return Fixnum::from_int(obj->value == right ? 0 : (obj->value > right ? 1 : -1)); });
	}
	
	void Float::initialize()
	{
		context->float_class = define_class("Float", context->numeric_class);
		
		method<Self<Float>, &to_s>(context->float_class, "to_s");
		method<Self<Value>, &to_f>(context->float_class, "to_f");
		method<Self<Float>, &zero>(context->float_class, "zero?");
		
		method<Self<Float>, Value, &add>(context->float_class, "+");
		method<Self<Float>, Value, &sub>(context->float_class, "-");
		method<Self<Float>, Value, &mul>(context->float_class, "*");
		method<Self<Float>, Value, &div>(context->float_class, "/");
		method<Self<Float>, Value, &compare>(context->float_class, "<=>");
	}
};

