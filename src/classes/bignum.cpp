#include "bignum.hpp"
#include "string.hpp"
#include "fixnum.hpp"
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
	
	void Bignum::initialize()
	{
		context->bignum_class = define_class("Bignum", context->integer_class);

		method<Arg::Self<Arg::Class<Bignum>>, Arg::Optional<Arg::Fixnum>, &to_s>(context->bignum_class, "to_s");
	}
};

