#include "io.hpp"
#include "string.hpp"
#include "fixnum.hpp"
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

	value_t rb_read(IO *io, intptr_t length)
	{
		io->assert_stream();

		CharArray result;
		
		if(length == Fixnum::undef)
			io->stream->read(result);
		else
		{
			if(length < 0)
				raise(context->argument_error, "Can't read a negative size");

			io->stream->read(result, (size_t)length);
		}

		return result.to_string();
	}

	void IO::initialize()
	{
		context->io_class = define_class("IO", context->object_class);
		
		method<Arg::Self<Arg::Class<IO>>, &rb_close>(context->io_class, "close");
		method<Arg::Self<Arg::Class<IO>>, Arg::Optional<Arg::Fixnum>, &rb_read>(context->io_class, "read");
		method<Arg::Self<Arg::Class<IO>>, Arg::Count, Arg::Values, &rb_print>(context->io_class, "print");
		method<Arg::Self<Arg::Class<IO>>, Arg::Count, Arg::Values, &rb_puts>(context->io_class, "puts");
	}
};

