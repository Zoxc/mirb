#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "../allocator.hpp"

namespace Mirb
{
	class ValueMap;

	class Object:
		public Value
	{
		private:
			static value_t tap(value_t obj, value_t block);
			static value_t klass(value_t obj);
			static value_t rb_dup(value_t obj);
			static value_t dummy();
			static value_t variadic_dummy(size_t argc);
			static value_t initialize_copy(value_t other);
			static value_t pattern(value_t);
			static value_t rb_inspect(value_t obj);
			static value_t instance_eval(value_t obj, String *string, value_t block);
			static value_t equal(value_t obj, value_t other);
			static value_t not_equal(value_t obj, value_t other);
			static value_t not_match(value_t obj, value_t other);
			static value_t rb_not(value_t obj);
			static value_t extend(value_t obj, size_t argc, value_t argv[]);
			static value_t kind_of(value_t obj, Class *klass);
			static value_t respond_to(value_t obj, Symbol *name);
			
			void generate_hash();
			
			Object(Type::Enum type) : Value(type), instance_of(nullptr), vars(nullptr) {}
			
			friend class Module;
			friend class Symbol;
		public:
			Object(const Object &other) : Value(other), instance_of(other.instance_of), vars(nullptr), hash_value(other.hash_value) {}

			Object(Type::Enum type, Class *instance_of) : Value(type), instance_of(instance_of), vars(nullptr)
			{
				verify(instance_of);
			}

			Object(Class *instance_of) : Value(Type::Object), instance_of(instance_of), vars(nullptr)
			{
				verify(instance_of);
			}
			
			static value_t allocate(Class *instance_of);
			static value_t to_s(value_t obj);

			union
			{
				Class *instance_of;
				Module *original_module;
			};

			ValueMap *vars;
			size_t hash_value;

			static void initialize();
			
			template<class T> static T *dup(T *other)
			{
				return new (collector) T(*other);
			}
			
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

