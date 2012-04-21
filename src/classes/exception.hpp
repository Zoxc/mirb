#pragma once
#include "object.hpp"

namespace Mirb
{
	class Exception:
		public Object
	{
		private:
			static value_t allocate(value_t obj);
			static value_t to_s(value_t obj);
			static value_t method_initialize(value_t obj, value_t message);

		public:
			Exception(Value::Type type, value_t instance_of, value_t message, value_t backtrace) : Object(type, instance_of), message(message), backtrace(backtrace) {}
			Exception(value_t instance_of, value_t message, value_t backtrace) : Object(Value::Exception, instance_of), message(message), backtrace(backtrace) {}
			
			value_t message;
			value_t backtrace;

			static value_t class_ref;

			static void initialize();
	};
};
