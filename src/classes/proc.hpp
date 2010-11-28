#pragma once
#include "object.hpp"

namespace Mirb
{
	class Proc:
		public Object
	{
		public:
			Proc(value_t instance_of, value_t self, Block *block, size_t scope_count, value_t **scopes) :
				Object(Value::Proc, instance_of),
				self(self),
				block(block),
				scope_count(scope_count),
				scopes(scopes)
			{
			}

			value_t self;
			Block *block;
			size_t scope_count;
			value_t **scopes;

			static value_t class_ref;

			static void initialize();
	};
};
