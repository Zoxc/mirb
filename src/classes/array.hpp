#pragma once
#include "object.hpp"
#include "../vector.hpp"

namespace Mirb
{
	class Array:
		public Object
	{
		public:
			Array(value_t instance_of) : Object(Value::Array, instance_of) {}
			Array() : Object(Value::Array, class_ref) {}

			Vector<value_t, GC> vector;
			
			static value_t class_ref;

			static void initialize();
	};
};
