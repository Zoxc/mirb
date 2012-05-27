#pragma once
#include "class.hpp"
#include "../number.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Bignum:
		public Object
	{
		private:
			Number number;

		public:
			Bignum(intptr_t value) : Object(Value::Bignum, context->bignum_class), number(value) {}
			Bignum(size_t value) : Object(Value::Bignum, context->bignum_class), number(value) {}
			
			static const bool finalizer = true;

			static void initialize();
	};
};
