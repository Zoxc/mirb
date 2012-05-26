#include "fixnum.hpp"
#include "class.hpp"
#include "string.hpp"
#include "float.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Fixnum::from_size_t(size_t value)
	{
		return from_int(value);
	}

	size_t Fixnum::to_size_t(value_t obj)
	{
		return to_int(obj);
	}
	
	value_t Fixnum::from_int(int_t value)
	{
		return (value_t)((value << 1) | 1);
	}

	Fixnum::int_t Fixnum::to_int(value_t obj)
	{
		return (Fixnum::int_t)obj >> 1;
	}
	
	value_t Fixnum::zero(int_t obj)
	{
		return Value::from_bool(obj == 0);
	}
	
	value_t Fixnum::to_s(int_t obj)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%d", (int)obj);

		return CharArray(buffer, length).to_string();
	}
	
	value_t Fixnum::to_f(int_t obj)
	{
		return Collector::allocate<Float>((double)obj);
	}
	
	value_t Fixnum::times(int_t obj, value_t block)
	{
		OnStack<1> os(block);

		if(obj <= 0)
			return from_int(obj);

		while(obj--)
			if(!yield(block))
				return 0;

		return from_int(obj);
	}
	
	value_t Fixnum::upto(int_t obj, int_t to, value_t block)
	{
		OnStack<1> os(block);

		for(int_t from = obj; from <= to; ++from)
		{
			value_t current = from_int(from);

			if(!yield_argv(block, 1, &current))
				return 0;
		}

		return from_int(obj);
	}
	
	template<typename F, size_t string_length> value_t coerce_op(intptr_t obj, value_t other, const char (&string)[string_length], F func)
	{
		if(prelude_likely(Value::is_fixnum(other)))
			return Fixnum::from_int(func(Fixnum::to_int(other)));
		else
			return coerce(Fixnum::from_int(obj), Symbol::get(string), other);
	}
	
	value_t Fixnum::add(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "+", [&](int_t right) { return obj + right; });
	}
	
	value_t Fixnum::sub(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "-", [&](int_t right) { return obj - right; });
	}
	
	value_t Fixnum::mul(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "*", [&](int_t right) { return obj * right; });
	}
	
	value_t Fixnum::div(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "/", [&](int_t right) { return obj / right; });
	}

	value_t Fixnum::mod(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "%", [&](int_t right) { return obj % right; });
	}

	value_t Fixnum::compare(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "<=>", [&](int_t right) { return obj == right ? 0 : (obj > right ? 1 : -1); });
	}
	
	value_t Fixnum::neg(int_t obj)
	{
		return from_int(-obj);
	}
	
	void Fixnum::initialize()
	{
		method<Arg::Self<Arg::Fixnum>>(context->fixnum_class, "to_s", &to_s);
		method<Arg::Self<Arg::Fixnum>>(context->fixnum_class, "to_f", &to_f);
		method<Arg::Self<Arg::Fixnum>>(context->fixnum_class, "zero?", &zero);
		method<Arg::Self<Arg::Fixnum>, Arg::Block>(context->fixnum_class, "times", &times);
		method<Arg::Self<Arg::Fixnum>, Arg::Fixnum, Arg::Block>(context->fixnum_class, "upto", &upto);
		
		method<Arg::Self<Arg::Fixnum>>(context->fixnum_class, "-@", &neg);

		method<Arg::Self<Arg::Fixnum>, Arg::Value>(context->fixnum_class, "+", &add);
		method<Arg::Self<Arg::Fixnum>, Arg::Value>(context->fixnum_class, "-", &sub);
		method<Arg::Self<Arg::Fixnum>, Arg::Value>(context->fixnum_class, "*", &mul);
		method<Arg::Self<Arg::Fixnum>, Arg::Value>(context->fixnum_class, "/", &div);
		method<Arg::Self<Arg::Fixnum>, Arg::Value>(context->fixnum_class, "%", &mod);
		method<Arg::Self<Arg::Fixnum>, Arg::Value>(context->fixnum_class, "<=>", &compare);
	}
};

