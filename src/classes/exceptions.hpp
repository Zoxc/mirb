#pragma once
#include "exception.hpp"

namespace Mirb
{
	void initialize_exceptions();
	
	class ReturnException:
		public Exception
	{
		public:
			ReturnException(Value::Type type, value_t instance_of, value_t message, value_t backtrace, Block *target, value_t value) : Exception(type, instance_of, message, backtrace), target(target), value(value) {}
			
			Block *target;
			value_t value;
			
			template<typename F> void mark(F mark)
			{
				Exception::mark(mark);

				mark(target);
				mark(value);
			}
	};
	
	class BreakException:
		public ReturnException
	{
		public:
			BreakException(value_t instance_of, value_t message, value_t backtrace, Block *target, value_t value, var_t dst) : ReturnException(Value::BreakException, instance_of, message, backtrace, target, value), dst(dst) {}
			
			var_t dst;
	};
};
