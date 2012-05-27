#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Bignum:
		public Object
	{
		private:
		public:
			Bignum() : Object(Value::Bignum, context->bignum_class) {}

			static void initialize();
	};
};
