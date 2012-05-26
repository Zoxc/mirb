#pragma once
#include "exception.hpp"

namespace Mirb
{
	void initialize_exceptions();
	
	class SystemStackError:
		public Exception
	{
		public:
			SystemStackError();
	};
	
	class NextException:
		public Exception
	{
		public:
			NextException(Value::Type type, Class *instance_of, value_t message, Tuple<StackFrame> *backtrace, value_t value) : Exception(type, instance_of, message, backtrace), value(value) {}
			
			value_t value;
			
			template<typename F> void mark(F mark)
			{
				Exception::mark(mark);

				mark(value);
			}
	};
	
	class RedoException:
		public Exception
	{
		public:
			RedoException(Value::Type type, Class *instance_of, value_t message, Tuple<StackFrame> *backtrace, size_t pos) : Exception(type, instance_of, message, backtrace), pos(pos) {}
			
			size_t pos;
	};
	
	class ReturnException:
		public NextException
	{
		public:
			ReturnException(Value::Type type, Class *instance_of, value_t message, Tuple<StackFrame> *backtrace, Block *target, value_t value) : NextException(type, instance_of, message, backtrace, value), target(target) {}
			
			Block *target;
			
			template<typename F> void mark(F mark)
			{
				NextException::mark(mark);

				mark(target);
			}
	};
	
	class BreakException:
		public ReturnException
	{
		public:
			BreakException(Class *instance_of, value_t message, Tuple<StackFrame> *backtrace, Block *target, value_t value, var_t dst) : ReturnException(Value::BreakException, instance_of, message, backtrace, target, value), dst(dst) {}
			
			var_t dst;
	};
};
