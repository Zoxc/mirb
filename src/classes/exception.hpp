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
			Tuple<Module> *scope;
			value_t block;

			Tuple<> *args;

			const char *ip;
			bool vars;
			
			static CharArray inspect_implementation(StackFrame *self);
			static void print(StackFrame *self);
		public:
			StackFrame(Frame *frame);
		
			template<typename F> void mark(F mark)
			{
				mark(code);
				mark(obj);
				mark(name);
				mark(scope);
				mark(block);
				mark(args);
			}
			
			static CharArray get_backtrace(Tuple<StackFrame> *backtrace);
			static void print_backtrace(Tuple<StackFrame> *backtrace);

			CharArray inspect();
	};

	class Exception:
		public Object
	{
		private:
			static value_t allocate(Class *instance_of);
			static value_t to_s(Exception *obj);
			static value_t rb_backtrace(Exception *self);
			static value_t rb_initialize(Exception *obj, String *message);

		public:
			Exception(Value::Type type, Class *instance_of, String *message, Tuple<StackFrame> *backtrace) : Object(type, instance_of), message(message), backtrace(backtrace) {}
			Exception(Class *instance_of, String *message, Tuple<StackFrame> *backtrace) : Object(Value::Exception, instance_of), message(message), backtrace(backtrace) {}
			
			String *message;
			Tuple<StackFrame> *backtrace;

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				if(message)
					mark(message);

				mark(backtrace);
			}

			static void initialize();
	};
};
