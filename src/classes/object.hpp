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

			static void initialize();

			template<typename F> void mark(F mark)
			{
				mark(instance_of);

				if(vars)
					mark(vars);
			}
	};
};

