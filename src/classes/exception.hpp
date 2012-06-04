#pragma once
#include "object.hpp"

namespace Mirb
{
	class Frame;
	class IO;
	class Stream;

	class StackFrame:
		public Value
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
			static CharArray inspect_plain_implementation(StackFrame *self);
			static void print(StackFrame *self, Stream &out);
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
			
			static Array *get_plain_backtrace(Tuple<StackFrame> *backtrace);
			static void print_backtrace(Tuple<StackFrame> *backtrace, Stream &out);

			CharArray inspect();
			CharArray inspect_plain();
	};

	class Exception:
		public Object
	{
		private:
			static value_t to_s(Exception *obj);
			static value_t rb_backtrace(Exception *self);
			static value_t rb_initialize(Exception *obj, String *message);
			static value_t rb_inspect(Exception *self);
			static value_t print(Exception *self, IO *io);
			
		protected:
			static void print_main(Exception *self, IO *io);
			static void print_backtrace(Exception *self, IO *io);

		public:
			Exception(Type::Enum type, Class *instance_of, String *message, Tuple<StackFrame> *backtrace) : Object(type, instance_of), message(message), backtrace(backtrace) {}
			Exception(Class *instance_of, String *message, Tuple<StackFrame> *backtrace) : Object(Type::Exception, instance_of), message(message), backtrace(backtrace) {}
			Exception(Class *instance_of) : Object(Type::Exception, instance_of), message(nullptr), backtrace(nullptr) {}
			
			String *message;
			Tuple<StackFrame> *backtrace;

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				if(message)
					mark(message);

				if(backtrace)
					mark(backtrace);
			}

			static void initialize();
	};
};
