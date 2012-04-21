#include "fixnum.hpp"
#include "object.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Fixnum::class_ref;
	
	static value_t int_to_fix(size_t imm)
	{
		return (value_t)((imm << 1) | 1);
	}
	
	static size_t fix_to_int(value_t obj)
	{
		return (size_t)obj >> 1;
	}
	
	value_t Fixnum::from_size_t(size_t value)
	{
		return int_to_fix(value);
	}

	size_t Fixnum::to_size_t(value_t obj)
	{
		return fix_to_int(obj);
	}
	
	value_t Fixnum::from_int(int value)
	{
		return int_to_fix((size_t)value);
	}

	int Fixnum::to_int(value_t obj)
	{
		return (int)fix_to_int(obj);
	}
	
	value_t Fixnum::to_s(value_t obj)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%d", fix_to_int(obj));

		return CharArray(buffer, length).to_string();
	}
	
	value_t Fixnum::times(value_t obj, value_t block)
	{
		size_t times = to_size_t(obj);

		while(times--)
			yield(block);

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

	void Fixnum::initialize()
	{
		static_method<Arg::Self>(Fixnum::class_ref, "to_s", &to_s);
		static_method<Arg::Self, Arg::Block>(Fixnum::class_ref, "times", &times);

		static_method<Arg::Self, Arg::Value>(Fixnum::class_ref, "+", &add);
		static_method<Arg::Self, Arg::Value>(Fixnum::class_ref, "-", &sub);
		static_method<Arg::Self, Arg::Value>(Fixnum::class_ref, "*", &mul);
		static_method<Arg::Self, Arg::Value>(Fixnum::class_ref, "/", &div);
	}
};

