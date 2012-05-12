#pragma once
#include "class.hpp"
#include "../value-map.hpp"

namespace Mirb
{
	class Hash:
		public Object
	{
		private:
			static value_t allocate(value_t obj);
			static value_t inspect(value_t obj);
			static value_t get(value_t obj, value_t key);
			static value_t set(value_t obj, value_t key, value_t value);

		public:
			Hash() : Object(Value::Hash, context->hash_class), map(3), default_value(value_nil) {}

			Hash(Class *instance_of) : Object(Value::Hash, instance_of), map(3), default_value(value_nil)
			{
				flag = false;
			}
			
			ValueMap::MapType map;

			value_t default_value;
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				map.mark(mark);
			}

			static void initialize();
	};
};
