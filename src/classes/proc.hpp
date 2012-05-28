#pragma once
#include "object.hpp"

namespace Mirb
{
	class Proc:
		public Object
	{
		private:
			static value_t rb_new(value_t block);

			friend class Object;

		public:
			Proc(Class *instance_of, value_t self, Symbol *name, Tuple<Module> *scope, Block *block, Tuple<> *scopes) :
				Object(Value::Proc, instance_of),
				self(self),
				name(name),
				scope(scope),
				block(block),
				scopes(scopes)
			{
			}
			
			static value_t call(Proc *self, value_t block, size_t argc, value_t argv[]);

			value_t self;
			Symbol *name;
			Tuple<Module> *scope;
			Block *block;
			Tuple<> *scopes;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
				
				mark(self);
				mark(name);
				mark(scope);
				mark(block);
				mark(scopes);
			}

			static void initialize();
	};
};
