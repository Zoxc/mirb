#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Float:
		public Object
	{
		private:
			static value_t neg(Float *obj);
			static value_t zero(Float *obj);
			static value_t to_s(Float *obj);
			static value_t to_f(value_t obj);
			static value_t add(Float *obj, value_t other);
			static value_t sub(Float *obj, value_t other);
			static value_t mul(Float *obj, value_t other);
			static value_t div(Float *obj, value_t other);
			static value_t compare(Float *obj, value_t other);

		public:
			Float(double value) : Object(Type::Float, context->float_class), value(value) {}

			static Float *allocate(double value);

			double value;

			static void initialize();
	};
};
