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
			Exception(Value::Type type, Module *instance_of, value_t message, value_t backtrace) : Object(type, instance_of), message(message), backtrace(backtrace) {}
			Exception(Module *instance_of, value_t message, value_t backtrace) : Object(Value::Exception, instance_of), message(message), backtrace(backtrace) {}
			
			value_t message;
			value_t backtrace;

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(message);
				mark(backtrace);
			}

			static void initialize();
	};
};
