#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "object.hpp"

namespace Mirb
{
	class Module:
		public Object
	{
		private:
			BlockMap *methods;

		public:
			Module(Value::Type type, value_t instance_of, value_t superclass) : Object(type, instance_of), superclass(superclass) {}

			value_t superclass;

			static const size_t methods_initial = 1;
			
			BlockMap *get_methods();

			static value_t class_ref;

			static void initialize();
	};
};

