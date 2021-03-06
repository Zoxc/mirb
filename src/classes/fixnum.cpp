#include "fixnum.hpp"
#include "class.hpp"
#include "string.hpp"
#include "float.hpp"
#include "bignum.hpp"
#include "array.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"

namespace Mirb
{
	bool Fixnum::fits(int_t value)
	{
		return (value >= low) && (value <= high);
	}
	
	value_t Fixnum::from_size_t(size_t value)
	{
		if(value > (size_t)high)
			return new (collector) Bignum(value);
		else
			return from_int(value);
	}

	size_t Fixnum::to_size_t(value_t obj)
	{
		return to_int(obj);
	}
	
	value_t Fixnum::convert(int_t value)
	{
		if(fits(value))
			return from_int(value);
		else
			return new (collector) Bignum(value);
	}

	value_t Fixnum::from_int(int_t value)
	{
		return (value_t)((value << 1) | 1);
	}

	Fixnum::int_t Fixnum::to_int(value_t obj)
	{
		return (Fixnum::int_t)obj >> 1;
	}
	
	value_t Fixnum::chr(int_t obj)
	{
		if(obj < 0 || obj > 255)
			raise(context->argument_error, "Invalid character point");

		return CharArray(obj).to_string();
	}
	
	value_t Fixnum::rb_size()
	{
		return Fixnum::from_int(sizeof(int_t));
	}
	
	value_t Fixnum::zero(int_t obj)
	{
		return Value::from_bool(obj == 0);
	}
	
	value_t Fixnum::to_s(int_t obj, int_t base)
	{
		return Bignum::to_string(Number(obj), base);
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
			yield(block);

		return from_int(obj);
	}
	
	value_t Fixnum::upto(int_t obj, int_t to, value_t block)
	{
		OnStack<1> os(block);

		for(int_t from = obj; from <= to; ++from)
			yield(block, from_int(from));

		return from_int(obj);
	}
	
	template<typename F, size_t string_length> value_t coerce_op(intptr_t obj, value_t other, const char (&string)[string_length], F func)
	{
		if(prelude_likely(Value::is_fixnum(other)))
			return func(Fixnum::to_int(other));
		else
			return coerce(Fixnum::from_int(obj), Symbol::get(string), other);
	}
	
	value_t Fixnum::add(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "+", [&](int_t right) { return Fixnum::convert(obj + right); });
	}
	
	value_t Fixnum::sub(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "-", [&](int_t right) { return Fixnum::convert(obj - right); });
	}
	
	const intptr_t sqrt_long_max = (intptr_t)1 << ((sizeof(intptr_t) * CHAR_BIT - 1) / 2);
	
	// Fixnum::mul is adopted from the Rubinius project. See LICENSE.rubinius

	value_t Fixnum::mul(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "*", [&](int_t right) -> value_t {
			auto fit_sqrt = [](int_t value)  { return (value < sqrt_long_max) && (value > -sqrt_long_max); };

			if(fit_sqrt(obj) && fit_sqrt(right))
				return Fixnum::from_int(obj * right);
			else
				return (Number(obj) * Number(right)).to_value();
		});
	}
	
	value_t Fixnum::div(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "/", [&](int_t right) {
			if(prelude_unlikely(right == 0))
				zero_division_error();

			return Fixnum::from_int(obj / right);
		});
	}

	value_t Fixnum::mod(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "%", [&](int_t right) {
			if(prelude_unlikely(right == 0))
				zero_division_error();

			return Fixnum::from_int(obj % right);
		});
	}

	value_t Fixnum::compare(int_t obj, value_t other)
	{
		return coerce_op(obj, other, "<=>", [&](int_t right) { return Fixnum::from_int(obj == right ? 0 : (obj > right ? 1 : -1)); });
	}
	
	value_t Fixnum::neg(int_t obj)
	{
		return from_int(-obj);
	}
	
	value_t Fixnum::invert(int_t obj)
	{
		return from_int(~obj);
	}
	
	value_t Fixnum::pow(int_t obj, int_t other)
	{
		if(other < 0)
			raise(context->argument_error, "Negative exponent not allowed with integers");

		if((size_t)other > (size_t)ULONG_MAX)
			raise(context->argument_error, "The exponent is too high");

		return Number(obj).pow((unsigned long)other).to_value();
	}

	value_t Fixnum::coerce(int_t obj, value_t other)
	{
		switch(other->type())
		{
			case Type::Bignum:
			{
				auto num = cast<Bignum>(other);

				if(num->number.can_be_fix())
					return Array::allocate_pair(from_int(num->number.to_intptr()), from_int(obj));
				else
					return Array::allocate_pair(num, Number(obj).to_bignum());
			}

			case Type::Fixnum:
				return Array::allocate_pair(other, from_int(obj));

			default:
				coerce_error(other, from_int(obj));
		};
	}

	// Fixnum::lshift is adopted from the Rubinius project. See LICENSE.rubinius

	value_t Fixnum::lshift(int_t obj, int_t shift)
	{
		if(shift < 0)
			return rshift(obj, -shift);

		int_t abs = obj < 0 ? -obj : obj;

		if((shift >= width) || ((abs >> (width - shift)) > 0))
			return Number(obj).lshift(shift).to_bignum();
		
		return from_int(obj << shift);
	}
	
	// Fixnum::rshift is adopted from the Rubinius project. See LICENSE.rubinius

	value_t Fixnum::rshift(int_t obj, int_t shift)
	{
		if(shift < 0)
			return lshift(obj, -shift);

		int_t abs = obj < 0 ? -obj : obj;

		if(shift >= bits)
			return from_int(obj >= 0 ? 0 : -1);
		
		return from_int(obj >> shift);
	}
	
	value_t Fixnum::bitwise_and(int_t obj, value_t other)
	{
		switch(other->type())
		{
			case Type::Bignum:
				return Number(obj).bitwise_and(cast<Bignum>(other)->number).to_value();

			case Type::Fixnum:
				return from_int(obj & to_int(other));

			default:
				type_error(other, "Fixnum or Bignum");
		};
	}
	
	value_t Fixnum::bitwise_xor(int_t obj, value_t other)
	{
		switch(other->type())
		{
			case Type::Bignum:
				return Number(obj).bitwise_xor(cast<Bignum>(other)->number).to_value();

			case Type::Fixnum:
				return from_int(obj ^ to_int(other));

			default:
				type_error(other, "Fixnum or Bignum");
		};
	}

	value_t Fixnum::bitwise_or(int_t obj, value_t other)
	{
		switch(other->type())
		{
			case Type::Bignum:
				return Number(obj).bitwise_or(cast<Bignum>(other)->number).to_value();

			case Type::Fixnum:
				return from_int(obj | to_int(other));

			default:
				type_error(other, "Fixnum or Bignum");
		};
	}

	void Fixnum::initialize()
	{
		method<Self<Arg::Fixnum>, Optional<Arg::Fixnum>, &to_s>(context->fixnum_class, "to_s");
		method<Self<Arg::Fixnum>, &to_f>(context->fixnum_class, "to_f");
		method<Self<Arg::Fixnum>, &zero>(context->fixnum_class, "zero?");
		method<Self<Arg::Fixnum>, Arg::Block, &times>(context->fixnum_class, "times");
		method<Self<Arg::Fixnum>, Arg::Fixnum, Arg::Block, &upto>(context->fixnum_class, "upto");
		method<Self<Arg::Fixnum>, &chr>(context->fixnum_class, "chr");
		method<&rb_size>(context->fixnum_class, "size");
		
		method<Self<Arg::Fixnum>, Arg::Fixnum, &lshift>(context->fixnum_class, "<<");
		method<Self<Arg::Fixnum>, Arg::Fixnum, &rshift>(context->fixnum_class, ">>");
		method<Self<Arg::Fixnum>, Value, &bitwise_and>(context->fixnum_class, "&");
		method<Self<Arg::Fixnum>, Value, &bitwise_xor>(context->fixnum_class, "^");
		method<Self<Arg::Fixnum>, Value, &bitwise_or>(context->fixnum_class, "|");

		method<Self<Arg::Fixnum>, &neg>(context->fixnum_class, "-@");
		method<Self<Arg::Fixnum>, &invert>(context->fixnum_class, "~");
		method<Self<Arg::Fixnum>, Arg::Fixnum, &pow>(context->fixnum_class, "**");

		method<Self<Arg::Fixnum>, Value, &add>(context->fixnum_class, "+");
		method<Self<Arg::Fixnum>, Value, &sub>(context->fixnum_class, "-");
		method<Self<Arg::Fixnum>, Value, &mul>(context->fixnum_class, "*");
		method<Self<Arg::Fixnum>, Value, &div>(context->fixnum_class, "/");
		method<Self<Arg::Fixnum>, Value, &mod>(context->fixnum_class, "%");
		method<Self<Arg::Fixnum>, Value, &compare>(context->fixnum_class, "<=>");
		method<Self<Arg::Fixnum>, Value, &coerce>(context->fixnum_class, "coerce");
	}
};

