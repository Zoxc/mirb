#include "stream.hpp"
#include "char-array.hpp"
#include "runtime.hpp"

namespace Mirb
{
	void Stream::puts(const CharArray &string)
	{
		print(string + "\n");
	}

	CharArray Stream::read(size_t length)
	{
		raise(context->io_error, "Reading not supported by stream");
	}

	void Stream::seek(pos_t val, PosType type)
	{
		raise(context->io_error, "Seeking not supported by stream");
	}

	Stream::pos_t Stream::pos()
	{
		raise(context->io_error, "Getting position not supported by stream");
	}

	Stream::pos_t Stream::size()
	{
		raise(context->io_error, "Getting size not supported by stream");
	}

	CharArray Stream::read()
	{
		pos_t remaining = size() - pos();

		mirb_debug_assert(remaining > 0);

		if(remaining > SIZE_MAX)
			raise(context->system_call_error, "Stream is too large to read");

		return read((size_t)remaining);
	}

	CharArray Stream::gets()
	{
		CharArray result, c;

		while(true)
		{
			c = read(1);

			if(!c.size())
				return result;

			result += c;

			if(c == "\n")
			{
				c = read(1);

				if(c == "\r")
					result += c;
				else
					seek(-c.size());

				return result;
			}
			else if (c == "\r")
				return result;
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

