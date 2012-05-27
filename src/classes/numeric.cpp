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

		if(!result)
			return 0;

		return Value::from_bool(!Value::test(result));
	}
	
	value_t Numeric::coerce(value_t obj, value_t other)
	{
		if(Value::type(obj) == Value::type(other))
			return Array::allocate_pair(other, obj);

		auto convert = [&](value_t input) -> value_t {
			if(Value::type(input) == Value::Float)
				return input;

			return raise_cast<Float>(call(input, "to_f"));
		};

		OnStack<1> os(obj);
		
		value_t left = convert(other);

		if(!left)
			return 0;

		value_t right = convert(obj);
		
		if(!right)
			return 0;

		return Array::allocate_pair(left, right);
	}

	void Numeric::initialize()
	{
		include_module(context->numeric_class, context->comparable_module);
		
		method<Arg::Self<Arg::Value>, &nonzero>(context->numeric_class, "nonzero?");
		method<Arg::Self<Arg::Value>, Arg::Value, &coerce>(context->numeric_class, "coerce");
	}
};

