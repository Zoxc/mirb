#include "comparable.hpp"
#include "../classes/fixnum.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	template<typename F> value_t compare(value_t obj, value_t other, F func)
	{
		value_t result = call(obj, context->syms.compare, value_nil, 1, &other);
		
		if(!result)
			return 0;

		if(!Value::is_fixnum(result))
			return raise(context->type_error, "<=> must return a Fixnum");

		return auto_cast(func(Fixnum::to_int(result)));
	}
	
	value_t Comparable::less(value_t obj, value_t other)
	{
		return compare(obj, other, [&](int v) { return v < 0; });
	}

	value_t Comparable::less_or_equal(value_t obj, value_t other)
	{
		return compare(obj, other, [&](int v) { return v <= 0; });
	}
	
	value_t Comparable::greater(value_t obj, value_t other)
	{
		return compare(obj, other, [&](int v) { return v > 0; });
	}
	
	value_t Comparable::greater_or_equal(value_t obj, value_t other)
	{
		return compare(obj, other, [&](int v) { return v >= 0; });
	}

	value_t Comparable::equal(value_t obj, value_t other)
	{
		return compare(obj, other, [&](int v) { return v == 0; });
	}

	value_t Comparable::between(value_t obj, value_t left, value_t right)
	{
		OnStack<2> os(obj, right);

		left = compare(obj, left, [&](int v) { return v >= 0; });

		if(!Value::test(left))
			return left;

		return compare(obj, left, [&](int v) { return v <= 0; });
	}

	void Comparable::initialize()
	{
		context->comparable_module = define_module("Comparable");

		method<Arg::Self, Arg::Value>(context->comparable_module, "<", &less);
		method<Arg::Self, Arg::Value>(context->comparable_module, "<=", &less_or_equal);
		method<Arg::Self, Arg::Value>(context->comparable_module, ">", &greater);
		method<Arg::Self, Arg::Value>(context->comparable_module, ">=", &greater_or_equal);
		method<Arg::Self, Arg::Value>(context->comparable_module, "==", &equal);
		method<Arg::Self, Arg::Value, Arg::Value>(context->comparable_module, "between?", &between);
	}
};
