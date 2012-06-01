#include "comparable.hpp"
#include "../classes/fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	template<typename F> value_t compare(value_t obj, value_t other, F func)
	{
		int result = compare(obj, other);
		
		return Value::from_bool(func(result));
	}
	
	value_t Comparable::less(value_t obj, value_t other)
	{
		return compare(obj, other, [&](intptr_t v) { return v < 0; });
	}

	value_t Comparable::less_or_equal(value_t obj, value_t other)
	{
		return compare(obj, other, [&](intptr_t v) { return v <= 0; });
	}
	
	value_t Comparable::greater(value_t obj, value_t other)
	{
		return compare(obj, other, [&](intptr_t v) { return v > 0; });
	}
	
	value_t Comparable::greater_or_equal(value_t obj, value_t other)
	{
		return compare(obj, other, [&](intptr_t v) { return v >= 0; });
	}

	value_t Comparable::equal(value_t obj, value_t other)
	{
		return compare(obj, other, [&](intptr_t v) { return v == 0; });
	}

	value_t Comparable::between(value_t obj, value_t left, value_t right)
	{
		OnStack<2> os(obj, right);

		left = compare(obj, left, [&](intptr_t v) { return v >= 0; });

		if(!left->test())
			return left;

		return compare(obj, left, [&](intptr_t v) { return v <= 0; });
	}

	void Comparable::initialize()
	{
		context->comparable_module = define_module("Comparable");

		method<Self<Value>, Value, &less>(context->comparable_module, "<");
		method<Self<Value>, Value, &less_or_equal>(context->comparable_module, "<=");
		method<Self<Value>, Value, &greater>(context->comparable_module, ">");
		method<Self<Value>, Value, &greater_or_equal>(context->comparable_module, ">=");
		method<Self<Value>, Value, &equal>(context->comparable_module, "==");
		method<Self<Value>, Value, Value, &between>(context->comparable_module, "between?");
	}
};
