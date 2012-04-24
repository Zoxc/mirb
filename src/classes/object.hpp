#pragma once
#include "../value.hpp"
#include "../gc.hpp"
#include "../block.hpp"
#include "../collector.hpp"

namespace Mirb
{
	class Object:
		public ObjectHeader
	{
		private:
			static value_t tap(value_t obj, value_t block);
			static value_t dummy();
			static value_t inspect(value_t obj);
			static value_t equal(value_t obj, value_t other);
			static value_t not_equal(value_t obj, value_t other);
			static value_t method_not(value_t obj);
		public:
			Object(Value::Type type, value_t instance_of) : ObjectHeader(type), instance_of(instance_of), vars(0) {}
			Object(value_t instance_of) : ObjectHeader(Value::Object), instance_of(instance_of), vars(0) {}
			
			static value_t allocate(value_t instance_of);
			static value_t to_s(value_t obj);

			static Block *inspect_block;
			
			value_t instance_of;
			ValueMap *vars;

			static const size_t vars_initial = 1;

			static value_t class_ref;

			static void initialize();
	};
};

