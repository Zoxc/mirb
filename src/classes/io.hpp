#pragma once
#include "class.hpp"
#include "../context.hpp"
#include "../platform/platform.hpp"

namespace Mirb
{
	class IO:
		public Object
	{
		private:
			static value_t rb_close(IO *io);
		public:
			IO(Platform::io_t handle, Class *instance_of) : Object(Value::IO, instance_of), handle(handle) {}

			~IO()
			{
				if(handle != Platform::invalid_io)
					Platform::close(handle);
			}

			void assert_handle();

			void close()
			{
				if(handle != Platform::invalid_io)
				{
					Platform::close(handle);
					handle = Platform::invalid_io;
				}
			}
			
			static const bool finalizer = true;

			Platform::io_t handle;

			template<typename F> void mark(F mark)
			{
				Object::mark(mark);
			}

			static void initialize();
	};
};
