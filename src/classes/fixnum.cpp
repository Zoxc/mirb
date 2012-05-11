#include "fixnum.hpp"
#include "class.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	static value_t int_to_fix(Fixnum::int_t imm)
	{
		return (value_t)((imm << 1) | 1);
	}
	
	static Fixnum::int_t fix_to_int(value_t obj)
	{
		return (Fixnum::int_t)obj >> 1;
	}
	
	value_t Fixnum::from_size_t(size_t value)
	{
		return int_to_fix(value);
	}

	size_t Fixnum::to_size_t(value_t obj)
	{
		return fix_to_int(obj);
	}
	
	value_t Fixnum::from_int(int_t value)
	{
		return int_to_fix((size_t)value);
	}

	Fixnum::int_t Fixnum::to_int(value_t obj)
	{
		return (int_t)fix_to_int(obj);
	}
	
	value_t Fixnum::zero(value_t obj)
	{
		return auto_cast(to_int(obj) == 0);
	}
	
	value_t Fixnum::to_s(value_t obj)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%d", (int)fix_to_int(obj));

		return CharArray(buffer, length).to_string();
	}
	
	value_t Fixnum::times(value_t obj, value_t block)
	{
		size_t times = to_size_t(obj);

		OnStack<2> os(obj, block);

		while(times--)
			if(!yield(block))
				return 0;

		return obj;
	}

	value_t Fixnum::add(value_t obj, value_t other)
	{
		return int_to_fix(fix_to_int(obj) + fix_to_int(other));
	}
	
	value_t Fixnum::sub(value_t obj, value_t other)
	{
		return int_to_fix(fix_to_int(obj) - fix_to_int(other));
	}
	
	value_t Fixnum::mul(value_t obj, value_t other)
	{
		return int_to_fix(fix_to_int(obj) * fix_to_int(other));
	}
	
	value_t Fixnum::div(value_t obj, value_t other)
	{
		return int_to_fix(fix_to_int(obj) / fix_to_int(other));
	}

	value_t Fixnum::compare(value_t obj, value_t other)
	{
		int_t lhs = fix_to_int(obj);
		int_t rhs = fix_to_int(other);

		return int_to_fix(lhs == rhs ? 0 : (lhs > rhs ? 1 : -1));
	}

	void Fixnum::initialize()
	{
		method<Arg::Self>(context->fixnum_class, "to_s", &to_s);
		method<Arg::Self>(context->fixnum_class, "zero?", &zero);
		method<Arg::Self, Arg::Block>(context->fixnum_class, "times", &times);

		method<Arg::Self, Arg::Value>(context->fixnum_class, "+", &add);
		method<Arg::Self, Arg::Value>(context->fixnum_class, "-", &sub);
		method<Arg::Self, Arg::Value>(context->fixnum_class, "*", &mul);
		method<Arg::Self, Arg::Value>(context->fixnum_class, "/", &div);
		method<Arg::Self, Arg::Value>(context->fixnum_class, "<=>", &compare);
	}
};

