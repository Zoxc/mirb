#pragma once
#include "object.hpp"

namespace Mirb
{
	class Frame;

	class StackFrame:
		public Value::Header
	{
		private:
			Block *code;
			value_t obj;
			Symbol *name;
			value_t module;
			value_t block;

			Tuple<> *args;

			const char *ip;
		public:
			StackFrame(Frame *frame);
		
			template<typename F> void mark(F mark)
			{
				mark(code);
				mark(obj);
				mark(name);
				mark(module);
				mark(block);
				mark(args);
			}
			
			static CharArray get_backtrace(Tuple<StackFrame> *backtrace);

			CharArray inspect();
	};

	class Exception:
		public Object
	{
		private:
			static value_t allocate(value_t obj);
			static value_t to_s(value_t obj);
			static value_t method_initialize(value_t obj, value_t message);

		public:
			Exception(Value::Type type, Module *instance_of, value_t message, Tuple<StackFrame> *backtrace) : Object(type, instance_of), message(message), backtrace(backtrace) {}
			Exception(Module *instance_of, value_t message, Tuple<StackFrame> *backtrace) : Object(Value::Exception, instance_of), message(message), backtrace(backtrace) {}
			
			value_t message;
			Tuple<StackFrame> *backtrace;

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(message);
				mark(backtrace);
			}

			static void initialize();
	};
};
