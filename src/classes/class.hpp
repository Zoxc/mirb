#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "module.hpp"

namespace Mirb
{
	class Class:
		public Module
	{
		private:
			static value_t to_s(value_t obj);
			static value_t method_superclass(value_t obj);
			static value_t method_new(value_t obj, size_t argc, value_t argv[]);

		public:
			Class(Value::Type type, value_t instance_of, value_t superclass, bool singleton = false) : Module(type, instance_of, superclass), singleton(singleton) {}
			Class(value_t module, value_t superclass);

			bool singleton;

			static value_t class_ref;
			
			template<typename F> void mark(F mark)
			{
				Module::mark(mark);
			}

			static void initialize();
	};
};

