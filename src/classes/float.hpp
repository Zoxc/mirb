#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Float:
		public Object
	{
		private:
			static value_t zero(Float *obj);
			static value_t to_s(Float *obj);
			static value_t add(Float *obj, Float *other);
			static value_t sub(Float *obj, Float *other);
			static value_t mul(Float *obj, Float *other);
			static value_t div(Float *obj, Float *other);
			static value_t compare(Float *obj, Float *other);

		public:
			Float(double value) : Object(Value::Float, context->float_class), value(value) {}

			double value;

			static void initialize();
	};
};
