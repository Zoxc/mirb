#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class IO:
		public Object
	{
		public:
			IO(Value::Type type, Class *instance_of) : Object(type, instance_of) {}

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
			}

			static void initialize();
	};
};
