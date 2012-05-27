#include "comparable.hpp"
#include "../classes/fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	template<typename F> value_t compare(value_t obj, value_t other, F func)
	{
		value_t result = compare(obj, other);
		
		if(!result)
			return 0;

		return Value::from_bool(func(Fixnum::to_int(result)));
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

		if(!Value::test(left))
			return left;

		return compare(obj, left, [&](intptr_t v) { return v <= 0; });
	}

	void Comparable::initialize()
	{
		context->comparable_module = define_module("Comparable");

		method<Arg::Self<Arg::Value>, Arg::Value, &less>(context->comparable_module, "<");
		method<Arg::Self<Arg::Value>, Arg::Value, &less_or_equal>(context->comparable_module, "<=");
		method<Arg::Self<Arg::Value>, Arg::Value, &greater>(context->comparable_module, ">");
		method<Arg::Self<Arg::Value>, Arg::Value, &greater_or_equal>(context->comparable_module, ">=");
		method<Arg::Self<Arg::Value>, Arg::Value, &equal>(context->comparable_module, "==");
		method<Arg::Self<Arg::Value>, Arg::Value, Arg::Value, &between>(context->comparable_module, "between?");
	}
};
