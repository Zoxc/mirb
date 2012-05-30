#pragma once
#include "class.hpp"
#include "../value-map.hpp"

namespace Mirb
{
	class Hash:
		public Object
	{
		private:
			static value_t allocate(Class *instance_of);
			static value_t to_s(Hash *self);
			static value_t keys(Hash *self);
			static value_t values(Hash *self);
			static value_t each(Hash *self, value_t block);
			static value_t get(Hash *self, value_t key);
			static value_t set(Hash *self, value_t key, value_t value);
			static value_t rb_initialize(Hash *obj, value_t def, value_t block);

		public:
			Hash() : Object(Value::Hash, context->hash_class), data(8), default_value(value_nil) {}

			Hash(Class *instance_of) : Object(Value::Hash, instance_of), data(8), default_value(value_nil)
			{
				flag = false;
				flag2 = false;
			}
			
			ValueMapData data;

			value_t default_value;
			
			bool accessing()
			{
				return flag2;
			}
			
			void accessing(bool value)
			{
				flag2 = value;
			}
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
				
				data.mark(mark);

				mark(default_value);
			}

			static void initialize();
	};
	
	typedef ValueMapManipulator<false, Hash, &Hash::data> HashAccess;
};
