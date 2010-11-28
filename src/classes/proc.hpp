#pragma once
#include "object.hpp"

namespace Mirb
{
	class Proc:
		public Object
	{
		public:
			Proc(value_t instance_of) : Object(Value::Proc, instance_of) {}
			Proc() : Object(Value::Proc, class_ref) {}
			
			value_t self;
			Block *block;
			size_t scope_count;
			value_t **scopes;

			static value_t class_ref;

			static void initialize();
	};
};
