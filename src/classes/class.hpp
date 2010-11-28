#pragma once
#include "../value.hpp"
#include "../block.hpp"
#include "module.hpp"

namespace Mirb
{
	class Class:
		public Module
	{
		public:
			Class(Value::Type type, value_t instance_of, value_t superclass, bool singleton = false) : Module(type, instance_of, superclass), singleton(singleton) {}
			Class(value_t module, value_t superclass);

			bool singleton;

			static value_t class_ref;

			static void initialize();
	};
};

