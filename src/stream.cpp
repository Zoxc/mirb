#include "stream.hpp"
#include "char-array.hpp"

namespace Mirb
{
	void Stream::puts(const CharArray &string)
	{
		print(string + "\n");
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

