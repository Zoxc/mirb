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
			static value_t klass(value_t obj);
			static value_t dummy();
			static value_t inspect(value_t obj);
			static value_t instance_eval(value_t obj, size_t argc, value_t argv[], value_t block);
			static value_t equal(value_t obj, value_t other);
			static value_t not_equal(value_t obj, value_t other);
			static value_t method_not(value_t obj);
			
			void generate_hash();
			
			Object(Value::Type type) : Value::Header(type), instance_of(nullptr), vars(nullptr) {}
			
			friend class Module;
			friend class Symbol;
		public:
			Object(Value::Type type, Class *instance_of) : Value::Header(type), instance_of(instance_of), vars(nullptr)
			{
				Value::verify(instance_of);
			}

			Object(Class *instance_of) : Value::Header(Value::Object), instance_of(instance_of), vars(nullptr)
			{
				Value::verify(instance_of);
			}
			
			static value_t allocate(value_t instance_of);
			static value_t to_s(value_t obj);

			union
			{
				Class *instance_of;
				Module *original_module;
			};

			ValueMap *vars;
			size_t hash_value;

			static void initialize();

			template<typename F> void mark(F mark)
			{
				mark(instance_of);

				if(vars)
					mark(vars);
			}
			
			size_t hash()
			{
				if(!hashed)
					generate_hash();

				return hash_value;
			}
	};
};

