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
			Proc(value_t instance_of, value_t self, Symbol *name, value_t module, Block *block, Tuple &scopes) :
				Object(Value::Proc, instance_of),
				self(self),
				name(name),
				module(module),
				block(block),
				scopes(scopes)
			{
			}

			value_t self;
			Symbol *name;
			value_t module;
			Block *block;
			Tuple &scopes;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
				
				mark(self);
				mark(name);
				mark(module);
				mark(block);
				mark(&scopes);
			}

			static void initialize();
	};
};
