#include "char-array.hpp"
#include "gc.hpp"
#include "classes/string.hpp"

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
	
	CharArray::CharArray(CharArray &&char_array)
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

	CharArray& CharArray::operator=(CharArray &other)
	{	
		if(this == &other)
			return *this;

		length = other.length;
		data = other.data;

		shared = true;
		other.shared = true;

		return *this;
	}
	
	CharArray& CharArray::operator=(CharArray &&other)
	{	
		if(this == &other)
			return *this;

		length = other.length;
		data = other.data;
		shared = other.shared;

		return *this;
	}

	void CharArray::localize()
	{
		if(shared)
		{
			char_t *new_data = (char_t *)gc.alloc(length);

			memcpy(new_data, data, length);

			data = new_data;
			shared = false;
		}
	}

	void CharArray::append(CharArray &&other)
	{
		CharArray &ref = other;
		append(ref);
	}
	
	void CharArray::append(CharArray& other)
	{
		localize();

		char_t *other_data = other.data;

		data = (char_t *)gc.realloc(data, length, length + other.length);
		memcpy(data + length, other_data, other.length);

		length += other.length;
	}

	CharArray &CharArray::operator+=(CharArray& other)
	{
		append(other);
		
		return *this;
	}
	
	CharArray &CharArray::operator+=(CharArray &&other)
	{
		append(other);
		
		return *this;
	}
	
	const char *CharArray::c_str_ref()
	{
		return (char *)data;
	}
	
	size_t CharArray::c_str_length()
	{
		return length - 1;
	}

	const char_t *CharArray::str_ref()
	{
		return data;
	}
	
	size_t CharArray::str_length()
	{
		return length - 1;
	}

	CharArray CharArray::c_str()
	{
		return *this + "\x00";
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
	
	CharArray operator+(CharArray &lhs, CharArray &rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}

	CharArray operator+(CharArray &&lhs, CharArray &rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}

	CharArray operator+(CharArray &lhs, CharArray &&rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}

	CharArray operator+(CharArray &&lhs, CharArray &&rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}

	value_t CharArray::to_string()
	{
		return auto_cast(new (gc) String(*this));
	}

	CharArray CharArray::hex(size_t value)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%x", value);

		return CharArray(buffer, length);
	}
};
