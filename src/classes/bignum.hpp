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

			static value_t to_s(Bignum *self, intptr_t base);

		public:
			Bignum(intptr_t value) : Object(Value::Bignum, context->bignum_class), number(value) {}
			Bignum(size_t value) : Object(Value::Bignum, context->bignum_class), number(value) {}
			Bignum(const Number &value) : Object(Value::Bignum, context->bignum_class), number(value) {}
			Bignum(Number &&value) : Object(Value::Bignum, context->bignum_class), number(value) {}
			
			static const bool finalizer = true;

			static void initialize();
	};
};
