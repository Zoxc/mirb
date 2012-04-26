#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "../allocator.hpp"

namespace Mirb
{
	class ValueMap;

	class Object:
		public Value::Header
	{
		private:
			static value_t tap(value_t obj, value_t block);
			static value_t dummy();
			static value_t inspect(value_t obj);
			static value_t equal(value_t obj, value_t other);
			static value_t not_equal(value_t obj, value_t other);
			static value_t method_not(value_t obj);
		public:
			Object(Value::Type type, Module *instance_of) : Value::Header(type), instance_of(instance_of), vars(0) {}
			Object(Module *instance_of) : Value::Header(Value::Object), instance_of(instance_of), vars(0) {}
			
			static value_t allocate(value_t instance_of);
			static value_t to_s(value_t obj);

			static Block *inspect_block;
			
			Module *instance_of;
			ValueMap *vars;

			static const size_t vars_initial = 2;
			
			static void initialize();

			template<typename F> void mark(F mark)
			{
				mark(instance_of);

				if(vars)
					mark(vars);
			}
	};

	class ValueMapFunctions:
		public MapFunctions<value_t, value_t>
	{
		public:
			static value_t invalid_value()
			{
				return value_raise;
			}
	};

	class ValueMap:
		public Value::Header
	{
		private:
		public:
			ValueMap() : Value::Header(Value::InternalValueMap), map(Object::vars_initial) {}

			Map<value_t, value_t, Allocator, ValueMapFunctions> map;
			
			template<typename F> void mark(F mark)
			{
				map.mark(mark);
			}
	};

};

