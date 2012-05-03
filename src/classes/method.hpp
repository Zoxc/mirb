#pragma once
#include "object.hpp"
#include "class.hpp"

namespace Mirb
{
	class Method:
		public Object
	{
		public:
			// Used as DSL for native methods
			enum Flags
			{
				Internal,
				Singleton,
				Private
			};

		public:
			Method(Class *instance_of) : Object(Value::Method, instance_of), block(0), scope(0) {}

			Method(Block *block, Tuple<Module> *scope) : Object(Value::Method, context->method_class), block(block), scope(scope) {}
			
			Block *block;
			Tuple<Module> *scope;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(block);

				if(scope)
					mark(scope);
			}

			static void initialize();
	};
};
