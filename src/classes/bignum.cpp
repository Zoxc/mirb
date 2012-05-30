#include "bignum.hpp"
#include "string.hpp"
#include "fixnum.hpp"
#include "array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Bignum::to_s(Bignum *self, intptr_t base)
	{
		if(base == Fixnum::undef)
			base = 10;

		if(base < 2 || base > 64)
			raise(context->argument_error, "Base must be in 2..64");

		return String::get(self->number.to_string());
	}
	
	template<typename F, size_t string_length> value_t coerce_op(Bignum *obj, value_t other, const char (&string)[string_length], F func)
	{
		Bignum *right = try_cast<Bignum>(other);

		if(prelude_likely(right != 0))
			return func(right->number);
		else
			return coerce(obj, Symbol::get(string), other);
	}
	
	value_t Bignum::neg(Bignum *obj)
	{
		return obj->number.neg().to_value();
	}

	value_t Bignum::add(Bignum *obj, value_t other)
	{
		return coerce_op(obj, other, "+", [&](const Number &right) { return (obj->number + right).to_value(); });
	}

	value_t Bignum::sub(Bignum *obj, value_t other)
	{
		return coerce_op(obj, other, "-", [&](const Number &right) { return (obj->number - right).to_value(); });
	}

	value_t Bignum::mul(Bignum *obj, value_t other)
	{
		return coerce_op(obj, other, "*", [&](const Number &right) { return (obj->number * right).to_value(); });
	}

	value_t Bignum::div(Bignum *obj, value_t other)
	{
		return coerce_op(obj, other, "/", [&](const Number &right) {
			
			if(prelude_unlikely(right == Number::zero))
				zero_division_error();

			return (obj->number / right).to_value();
		});
	}

	value_t Bignum::mod(Bignum *obj, value_t other)
	{
		return coerce_op(obj, other, "%", [&](const Number &right) {
			if(prelude_unlikely(right == Number::zero))
				zero_division_error();

			return (obj->number.mod(right)).to_value();
		});
	}

	value_t Bignum::compare(Bignum *obj, value_t other)
	{
		return coerce_op(obj, other, "<=>", [&](const Number &right) { return Fixnum::from_int(obj->number.compare(Number(right))); });
	}
	
	value_t Bignum::zero(Bignum *obj)
	{
		return Value::from_bool(obj->number == Number::zero);
	}
	
	value_t Bignum::coerce(Bignum *obj, value_t other)
	{
		switch(Value::type(other))
		{
			case Value::Fixnum:
			{
				if(obj->number.can_be_fix())
					return Array::allocate_pair(other, Fixnum::from_int(obj->number.to_intptr()));
				else
					return Array::allocate_pair(Number(Fixnum::to_int(other)).to_bignum(), obj);
			}
			
			case Value::Bignum:
				return Array::allocate_pair(other, obj);

			default:
				coerce_error(other, obj);
		};
	}

	void Bignum::initialize()
	{
		context->bignum_class = define_class("Bignum", context->integer_class);

		method<Arg::Self<Arg::Class<Bignum>>, Arg::Optional<Arg::Fixnum>, &to_s>(context->bignum_class, "to_s");
		
		method<Arg::Self<Arg::Class<Bignum>>, &zero>(context->bignum_class, "zero?");
		method<Arg::Self<Arg::Class<Bignum>>, &neg>(context->bignum_class, "-@");

		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &add>(context->bignum_class, "+");
		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &sub>(context->bignum_class, "-");
		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &mul>(context->bignum_class, "*");
		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &div>(context->bignum_class, "/");
		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &mod>(context->bignum_class, "%");
		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &compare>(context->bignum_class, "<=>");
		method<Arg::Self<Arg::Class<Bignum>>, Arg::Value, &coerce>(context->bignum_class, "coerce");
	}
};

