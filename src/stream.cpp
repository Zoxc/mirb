#include "stream.hpp"
#include "char-array.hpp"
#include "runtime.hpp"

namespace Mirb
{
	void Stream::puts(const CharArray &string)
	{
		print(string + "\n");
	}

	CharArray Stream::read(size_t)
	{
		raise(context->io_error, "Reading not supported by stream");
	}

	void Stream::seek(pos_t, PosType)
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

		if((size_t)remaining > SIZE_MAX)
			raise(context->system_call_error, "Stream is too large to read");

		return read((size_t)remaining);
	}

	CharArray Stream::gets()
	{
		static const size_t buffer_size = 0x200;

		CharArray result, buffer;

		while(true)
		{
			pos_t p = pos();
			buffer = read(buffer_size);

			for(size_t i = 0; i < buffer.size(); ++i)
			{
				if(buffer[i] == '\r')
				{
					++i;

					if(i < buffer.size())
					{
						if(buffer[i] == '\n')
							++i;

						seek(p + i, FromStart);
						return result + buffer.copy(0, i);
					}
					else
					{
						CharArray c = read(1);

						if(c == "\n")
							buffer += c;
						else
							seek(-(pos_t)c.size());

						return result + buffer;
					}
				}
				else if(buffer[i] == '\n')
				{
					++i;

					seek(p + i, FromStart);
					return result + buffer.copy(0, i);
				}
			}

			if(buffer.size() < buffer_size)
				return result + buffer;

			result += buffer;
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

