#pragma once
#include "../value.hpp"
#include "../gc.hpp"
#include "../generic/map.hpp"
#include "../block.hpp"

namespace Mirb
{
	class Object
	{
		public:
			Object(Value::Type type, value_t instance_of) : type(type), instance_of(instance_of) {}
			Object(value_t instance_of) : type(Value::Object) {}
			
			static value_t allocate(value_t instance_of);
			
			const Value::Type type;
			value_t instance_of;
			ValueMap *vars;

			static const size_t vars_initial = 1;

			static value_t class_ref;

			static void initialize();
	};

	mirb_compiled_block(object_to_s);
};

