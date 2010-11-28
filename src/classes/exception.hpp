#pragma once
#include "object.hpp"

namespace Mirb
{
	class Exception:
		public Object
	{
		public:
			Exception(value_t instance_of, value_t message = value_nil, value_t backtrace = value_nil) : Object(Value::Exception, instance_of), message(message), backtrace(backtrace) {}
			Exception(value_t message, value_t backtrace) : Object(Value::Exception, class_ref), message(message), backtrace(backtrace) {}
			
			value_t message;
			value_t backtrace;

			static value_t class_ref;

			static void initialize();
	};
};
