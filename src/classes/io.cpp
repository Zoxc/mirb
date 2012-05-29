#include "io.hpp"
#include "string.hpp"
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
	
	value_t IO::rb_print(IO *io, size_t argc, value_t argv[])
	{
		io->assert_stream();
		
		for(size_t i = 0; i < argc; ++i)
			io->stream->print(cast_string(argv[i])->string);

		return value_nil;
	}

	value_t IO::rb_puts(IO *io, size_t argc, value_t argv[])
	{
		io->assert_stream();
		
		for(size_t i = 0; i < argc; ++i)
			io->stream->puts(cast_string(argv[i])->string);

		return value_nil;
	}

	void IO::initialize()
	{
		context->io_class = define_class("IO", context->object_class);
		
		method<Arg::Self<Arg::Class<IO>>, &rb_close>(context->io_class, "close");
		method<Arg::Self<Arg::Class<IO>>, Arg::Count, Arg::Values, &rb_print>(context->io_class, "print");
		method<Arg::Self<Arg::Class<IO>>, Arg::Count, Arg::Values, &rb_puts>(context->io_class, "puts");
	}
};

