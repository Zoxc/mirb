#pragma once
#include "class.hpp"
#include "../context.hpp"

namespace Mirb
{
	class Stream;

	class IO:
		public Object
	{
		private:
			static value_t rb_close(IO *io);
			static value_t rb_print(IO *io, size_t argc, value_t argv[]);
			static value_t rb_puts(IO *io, size_t argc, value_t argv[]);
		public:
			IO(Stream *stream, Class *instance_of, bool owner = true) : Object(Type::IO, instance_of), stream(stream) { flag = owner; }
			~IO();

			Stream *stream;

			void assert_stream();
			void close();
			
			static const bool finalizer = true;

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
			}

			static void initialize();
	};
};
