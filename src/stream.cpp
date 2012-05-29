#include "stream.hpp"
#include "char-array.hpp"
#include "runtime.hpp"

namespace Mirb
{
	void Stream::puts(const CharArray &string)
	{
		print(string + "\n");
	}

	void Stream::read(CharArray &out, size_t length)
	{
		raise(context->system_call_error, "Reading not supported by stream");
	}

	void Stream::seek(pos_t val, PosType type)
	{
		raise(context->system_call_error, "Seeking not supported by stream");
	}

	Stream::pos_t Stream::pos()
	{
		raise(context->system_call_error, "Getting position not supported by stream");
	}

	Stream::pos_t Stream::size()
	{
		raise(context->system_call_error, "Getting size not supported by stream");
	}

	void Stream::read(CharArray &out)
	{
		pos_t remaining = size() - pos();

		mirb_debug_assert(remaining > 0);

		if(remaining > SIZE_MAX)
			raise(context->system_call_error, "Stream is too large to read");

		read(out, (size_t)remaining);
	}

	void Stream::gets(CharArray &out)
	{
		CharArray c;

		while(true)
		{
			read(c, 1);

			if(!c.size())
				return;

			out += c;

			if(c == "\n")
			{
				read(c, 1);

				if(c != "\r")
					seek(-1);
			}
			else if (c == "\r")
				return;
		}
	}

	void Stream::color(Color, const CharArray &string)
	{
		print(string);
	}
	
	CharArrayStream::CharArrayStream(CharArray &str) : str(str), os(str) {}

	void CharArrayStream::print(const CharArray &string)
	{
		str += string;
	}

};

