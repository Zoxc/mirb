#include "char-array.hpp"
#include "classes/string.hpp"
#include "collector.hpp"

namespace Mirb
{
	CharArray::CharArray(const char_t *c_str)
	{
		*this = c_str;
	}
	
	CharArray::CharArray(const char_t *c_str, size_t length)
	{
		this->length = length;

		data = (char_t *)Collector::Allocator::allocate(length);

		memcpy(data, c_str, length);

		shared = false;
	}

	CharArray::CharArray(const std::string &string)
	{
		*this = string;
	}
	
	CharArray::CharArray(const CharArray &char_array)
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

		data = (char_t *)Collector::Allocator::allocate(length);

		memcpy(data, c_str, length);

		shared = false;

		return *this;
	}

	CharArray& CharArray::operator=(const std::string &string)
	{
		length = string.length();

		data = (char_t *)Collector::Allocator::allocate(length);

		memcpy(data, string.c_str(), length);

		shared = false;

		return *this;
	}

	CharArray& CharArray::operator=(const CharArray &other)
	{	
		if(prelude_unlikely(this == &other))
			return *this;

		length = other.length;
		data = other.data;

		shared = true;
		other.shared = true;

		return *this;
	}
	
	CharArray& CharArray::operator=(CharArray &&other)
	{	
		if(prelude_unlikely(this == &other))
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
			char_t *new_data = (char_t *)Collector::Allocator::allocate(length);

			memcpy(new_data, data, length);

			data = new_data;
			shared = false;
		}
	}

	void CharArray::append(const CharArray& other)
	{
		// Note: other may be equal to *this

		char_t *this_data = data;
		char_t *other_data = other.data;

		if(shared)
		{
			data = (char_t *)Collector::Allocator::allocate(length + other.length);
			memcpy(data, this_data, length);
		}
		else
			data = (char_t *)Collector::Allocator::reallocate(data, length, length + other.length);
		
		memcpy(data + length, other_data, other.length);

		length += other.length;
	}

	CharArray &CharArray::operator+=(const CharArray& other)
	{
		append(other);
		
		return *this;
	}
	
	size_t CharArray::size() const
	{
		return length;
	}

	const char_t *CharArray::raw() const
	{
		return data;
	}

	const char *CharArray::c_str_ref() const
	{
		return (char *)data;
	}
	
	size_t CharArray::c_str_length() const
	{
		return length - 1;
	}

	const char_t *CharArray::str_ref() const
	{
		return data;
	}
	
	size_t CharArray::str_length() const
	{
		return length - 1;
	}

	CharArray CharArray::c_str() const
	{
		return *this + "\x00";
	}
			
	size_t CharArray::hash() const
	{
		size_t hash = 0;

		const char_t *start = data;
		const char_t *stop = data + length;

		for(const char_t *c = start; c != stop; c++)
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
	
	CharArray operator+(const CharArray &lhs, const CharArray &rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}

	value_t CharArray::to_string() const
	{
		return auto_cast(Collector::allocate<String>(*this));
	}
	
	CharArray CharArray::hex(size_t value)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%x", (unsigned int)value);

		return CharArray(buffer, length);
	}

	CharArray CharArray::uint(size_t value)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%u", (unsigned int)value);

		return CharArray(buffer, length);
	}

	CharArray operator *(CharArray lhs, size_t rhs)
	{
		CharArray result;

		for(size_t i = 0; i < rhs; ++i)
			result += lhs;

		return result;
	}
};
