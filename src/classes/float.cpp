#include "float.hpp"
#include "../runtime.hpp"
#include "../platform/platform.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include <limits>

namespace Mirb
{
	Float *Float::allocate(double value)
	{
		return new (collector) Float(value);
	}

	value_t Float::to_s(Float *obj)
	{
		std::stringstream buffer;
		buffer << obj->value;
		return CharArray(buffer.str()).to_string();
	}
	
	value_t Float::to_f(value_t obj)
	{
		return obj;
	}
	
	value_t Float::zero(Float *obj)
	{
		return Value::from_bool(obj->value == 0.0);
	}
	
	value_t Float::neg(Float *obj)
	{
		return allocate(-obj->value);
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
		return coerce_op(obj, other, "+", [&](double right) { return allocate(obj->value + right); });
	}
	
	value_t Float::sub(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "-", [&](double right) { return allocate(obj->value - right); });
	}
	
	value_t Float::mul(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "*", [&](double right) { return allocate(obj->value * right); });
	}
	
	value_t Float::div(Float *obj, value_t other)
	{
		return coerce_op(obj, other, "/", [&](double right) {
			if(prelude_unlikely(right == 0.0))
				zero_division_error();

			return allocate(obj->value / right);
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
		method<Self<Float>, &neg>(context->float_class, "-@");

		method<Self<Float>, Value, &add>(context->float_class, "+");
		method<Self<Float>, Value, &sub>(context->float_class, "-");
		method<Self<Float>, Value, &mul>(context->float_class, "*");
		method<Self<Float>, Value, &div>(context->float_class, "/");
		method<Self<Float>, Value, &compare>(context->float_class, "<=>");
		
		set_const(context->float_class, "MIN", Float::allocate(std::numeric_limits<double>::min())); 
		set_const(context->float_class, "MAX", Float::allocate(std::numeric_limits<double>::max())); 
	}
};

