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
			Hash() : Object(Value::Hash, context->hash_class), data(8), default_value(value_nil), _accessing(false) {}

			Hash(Class *instance_of) : Object(Value::Hash, instance_of), data(8), default_value(value_nil), _accessing(false)
			{
				flag = false;
			}
			
			ValueMapData data;

			value_t default_value;
			bool _accessing;
			
			bool accessing()
			{
				return _accessing;
			}
			
			void accessing(bool value)
			{
				_accessing = value;
			}
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				data.mark(mark);
			}

			static void initialize();
	};
	
	typedef ValueMapManipulator<false, Hash, &Hash::data> HashAccess;
};
