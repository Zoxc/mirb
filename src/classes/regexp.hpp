#pragma once
#include "../char-array.hpp"
#include "../collector.hpp"
#include "../context.hpp"
#include "class.hpp"

namespace Mirb
{
	class Regexp:
		public Object
	{
		public:
			Regexp(Class *instance_of) : Object(Value::Regexp, instance_of) {}

			static void initialize();
	};
};
