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
			Method(Class *instance_of) : Object(Value::Method, instance_of), block(0), module(0) {}

			Method(Block *block, Module *module) : Object(Value::Method, context->method_class), block(block), module(module) {}
			
			Block *block;
			Module *module;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(block);
				mark(module);
			}

			static void initialize();
	};
};
