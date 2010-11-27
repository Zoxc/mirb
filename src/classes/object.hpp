#pragma once
#include "../value.hpp"
#include "../gc.hpp"
#include "../generic/map.hpp"

namespace Mirb
{
	class Object
	{
		public:
			Value::Type type;
			value_t instance_of;
			ValueMap *vars;

			static const size_t vars_initial = 1;

			static value_t class_ref;
	};

};

