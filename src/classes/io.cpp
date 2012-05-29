#include "io.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	void IO::assert_handle()
	{
		if(handle == Platform::invalid_io)
			raise(context->io_error, "IO object is closed");
	}

	value_t IO::rb_close(IO *io)
	{
		io->close();
		return value_nil;
	}
	
	void IO::initialize()
	{
		context->io_class = define_class("IO", context->object_class);

		method<Arg::Self<Arg::Class<IO>>, &rb_close>(context->io_class, "close");
	}
};

