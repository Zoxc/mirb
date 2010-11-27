#pragma once
#include "../value.hpp"
#include "../gc.hpp"
#include "../generic/map.hpp"

namespace Mirb
{
	class Object
	{
		public:
			Object(Value::Type type, value_t instance_of) : type(type), instance_of(instance_of) {}
			Object() : type(Value::Object), instance_of(Object::class_ref) {}
			
			static value_t allocate();
			
			const Value::Type type;
			value_t instance_of;
			ValueMap *vars;

			static const size_t vars_initial = 1;

			static value_t class_ref;
	};

};

