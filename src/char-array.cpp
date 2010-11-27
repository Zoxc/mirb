#include "char-array.hpp"
#include "gc.hpp"

namespace Mirb
{
	CharArray::CharArray(const char_t *c_str)
	{
		*this = c_str;
	}
	
	CharArray::CharArray(const char_t *c_str, size_t length)
	{
		this->length = length;

		data = (char_t *)gc.alloc(length);

		memcpy(data, c_str, length);

		shared = false;
	}

	CharArray::CharArray(const std::string &string)
	{
		*this = string;
	}

	CharArray::CharArray(CharArray &char_array)
	{
		*this = char_array;
	}
	
	CharArray& CharArray::operator=(const char_t *c_str)
	{
		length = std::strlen((const char *)c_str);

		data = (char_t *)gc.alloc(length);

		memcpy(data, c_str, length);

		shared = false;

		return *this;
	}

	CharArray& CharArray::operator=(const std::string &string)
	{
		length = string.length();

		data = (char_t *)gc.alloc(length);

		memcpy(data, string.c_str(), length);

		shared = false;

		return *this;
	}

	CharArray& CharArray::operator=(CharArray& other)
	{	
		if(this == &other)
			return *this;

		length = other.length;
		data = other.data;

		shared = true;
		other.shared = true;

		return *this;
	}
	
	size_t CharArray::hash()
	{
		size_t hash = 0;

		const char_t *start = data;
		const char_t *stop = data + length;

		for(const char_t *c = start; c < stop; c++)
		{
			hash += *c;
			hash += hash << 10;
			hash ^= hash >> 6;
		}

		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;

		return hash;
	}
	
};
