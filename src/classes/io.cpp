#include "io.hpp"
#include "../runtime.hpp"
#include "../stream.hpp"

namespace Mirb
{
	void IO::assert_stream()
	{
		if(!stream)
			raise(context->io_error, "IO object is closed");
	}
	
	IO::~IO()
	{
		if(stream && flag)
			delete stream;
	}
	
	void IO::close()
	{
		if(stream)
		{
			if(flag)
				delete stream;
			stream = 0;
		}
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

