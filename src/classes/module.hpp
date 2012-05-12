#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "object.hpp"

namespace Mirb
{
	class HashMap;

	class Module:
		public Object
	{
		private:
			static value_t method_defined(Module *obj, Symbol *name);
			static value_t const_defined(Module *obj, Symbol *constant);
			static value_t const_get(Module *obj, Symbol *constant);
			static value_t const_set(Module *obj, Symbol *constant, value_t value);
			static value_t to_s(value_t obj);
			static value_t append_features(value_t obj, value_t mod);
			static value_t included(value_t obj);
			static value_t extend_object(value_t self, value_t obj);
			static value_t module_function(Module *obj, size_t argc, value_t argv[]);
			
			Module(Value::Type type) : Object(type), methods(nullptr), superclass(nullptr) {}

			friend class Class;
		protected:
			HashMap *methods;

		public:
			Module(Value::Type type, Class *instance_of, Class *superclass) : Object(type, instance_of), methods(nullptr), superclass(superclass) {}

			Class *superclass;

			static value_t include(value_t obj, size_t argc, value_t argv[]);

			HashMap *get_methods();
			
			Method *get_method(Symbol *name);
			void set_method(Symbol *name, Method *method);

			static value_t alias_method(Module *obj, Symbol *new_name, Symbol *old_name);

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
				
				if(superclass)
					mark(superclass);

				if(methods)
					mark(methods);
			}

			static void initialize();
	};
};

