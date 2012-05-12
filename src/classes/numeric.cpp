#include "numeric.hpp"
#include "class.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Numeric::nonzero(value_t obj)
	{
		value_t result = call(obj, "zero?");

		if(!result)
			return 0;

		return auto_cast(!Value::test(result));
	}

	void Numeric::initialize()
	{
		include_module(context->numeric_class, context->comparable_module);
		
		method<Arg::Self>(context->numeric_class, "nonzero?", &nonzero);
	}
};

