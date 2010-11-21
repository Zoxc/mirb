#pragma once
#include "../value.hpp"
#include "../gc.hpp"
#include "../map.hpp"

namespace Mirb
{
	class Object
	{
		public:
			Value::Type type;
			value_t instance_of;
			ValueMap *ivars;
	};
};

