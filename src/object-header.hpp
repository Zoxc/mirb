#pragma once
#include "value.hpp"

namespace Mirb
{
	class ObjectHeader
	{
		private:
			const Value::Type type;

		public:
			ObjectHeader(Value::Type type) : type(type) {}
			
			Value::Type get_type();
	};
};

