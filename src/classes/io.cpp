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

		if(length == Fixnum::undef)
			return io->stream->read().to_string();
		else
		{
			if(length < 0)
				raise(context->argument_error, "Can't read a negative size");

			return io->stream->read((size_t)length).to_string();
		}
	}
	
	value_t rb_gets(IO *io)
	{
		io->assert_stream();

		auto result = io->stream->gets();

		if(result.size())
			return result.to_string();
		else
			return value_nil;
	}
	
	value_t rb_each_line(IO *io, value_t block)
	{
		OnStack<2> os(io, block);

		while(true)
		{
			io->assert_stream();

			CharArray line = io->stream->gets();

			if(line.size())
				yield(block, line.to_string());
			else
				break;
		}

		return io;
	}

	void IO::initialize()
	{
		context->io_class = define_class("IO", context->object_class);
		
		method<Self<IO>, &rb_close>(context->io_class, "close");
		method<Self<IO>, Optional<Arg::Fixnum>, &rb_read>(context->io_class, "read");
		method<Self<IO>, Arg::Block, &rb_each_line>(context->io_class, "each_line");
		method<Self<IO>, Arg::Count, Arg::Values, &rb_print>(context->io_class, "print");
		method<Self<IO>, Arg::Count, Arg::Values, &rb_puts>(context->io_class, "puts");
		method<Self<IO>, &rb_gets>(context->io_class, "gets");
	}
};

