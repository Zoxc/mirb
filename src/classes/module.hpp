#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "object.hpp"

namespace Mirb
{
	class Module:
		public Object
	{
		private:
			static value_t to_s(value_t obj);
			static value_t append_features(value_t obj, value_t mod);
			static value_t included(value_t obj);
			
		protected:
			ValueMap *methods;

		public:
			Module(Value::Type type, value_t instance_of, value_t superclass) : Object(type, instance_of), methods(0), superclass(superclass) {}

			value_t superclass;

			static value_t include(value_t obj, size_t argc, value_t argv[]);

			ValueMap *get_methods();

			static value_t class_ref;

			static void initialize();
	};
};

