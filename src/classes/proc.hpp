#pragma once
#include "object.hpp"

namespace Mirb
{
	class Proc:
		public Object
	{
		private:
			static value_t call(value_t obj, value_t block, size_t argc, value_t argv[]);

		public:
			Proc(value_t instance_of, value_t self, Symbol *name, value_t module, Block *block, size_t scope_count, value_t **scopes) :
				Object(Value::Proc, instance_of),
				self(self),
				name(name),
				module(module),
				block(block),
				scope_count(scope_count),
				scopes(scopes)
			{
			}

			value_t self;
			Symbol *name;
			value_t module;
			Block *block;
			size_t scope_count;
			value_t **scopes;

			static value_t class_ref;

			static void initialize();
	};
};
