#include "bignum.hpp"
#include "../runtime.hpp"
#include <tommath.h>

namespace Mirb
{
	void Bignum::initialize()
	{
		context->bignum_class = define_class("Bignum", context->integer_class);
	}
};

