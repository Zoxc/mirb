#include "numeric.hpp"
#include "class.hpp"
#include "array.hpp"
#include "float.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Numeric::nonzero(value_t obj)
	{
		value_t result = call(obj, "zero?");

		return Value::from_bool(!result->test());
	}
	
	value_t Numeric::coerce(value_t obj, value_t other)
	{
		if(obj->type() == other->type())
			return Array::allocate_pair(other, obj);

		auto convert = [&](value_t input) -> value_t {
			if(input->type() == Type::Float)
				return input;

			return raise_cast<Float>(call(input, "to_f"));
		};

		OnStack<1> os(obj);
		
		value_t left = convert(other);
		value_t right = convert(obj);
		
		return Array::allocate_pair(left, right);
	}
	
	value_t Numeric::pos(value_t obj)
	{
		return obj;
	}
	
	void Numeric::initialize()
	{
		include_module(context->numeric_class, context->comparable_module);
		
		method<Self<Value>, &pos>(context->integer_class, "+@");
		method<Self<Value>, &nonzero>(context->numeric_class, "nonzero?");
		method<Self<Value>, Value, &coerce>(context->numeric_class, "coerce");
	}
};

