#include "char-array.hpp"
#include "classes/string.hpp"
#include "collector.hpp"

namespace Mirb
{
	CharArray::CharArray(const char_t *c_str)
	{
		*this = c_str;
	}
	
	CharArray::CharArray(const char_t c)
	{
		this->length = 1;

		data = (char_t *)Collector::allocate(1);

		*data = c;

		shared = false;
		static_data = false;
	}

	CharArray::CharArray(const char_t *c_str, size_t length)
	{
		this->length = length;

		data = (char_t *)Collector::allocate(length);

		memcpy(data, c_str, length);

		shared = false;
		static_data = false;
	}
	
	void CharArray::buffer(size_t size)
	{
		length = size;

		data = (char_t *)Collector::allocate(size);

		shared = false;
		static_data = false;
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

		data = (char_t *)Collector::allocate(length);

		memcpy(data, c_str, length);

		shared = false;
		static_data = false;

		return *this;
	}

	CharArray& CharArray::operator=(const std::string &string)
	{
		length = string.length();

		data = (char_t *)Collector::allocate(length);

		memcpy(data, string.c_str(), length);

		shared = false;
		static_data = false;

		return *this;
	}

	CharArray& CharArray::operator=(const CharArray &other)
	{	
		if(prelude_unlikely(this == &other))
			return *this;

		length = other.length;
		data = other.data;
		static_data = other.static_data;

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
		static_data = other.static_data;

		return *this;
	}

	int CharArray::to_i()
	{
		CharArray str = c_str();

		return atoi(str.c_str_ref());
	}
	
	CharArray CharArray::ljust(size_t length, const CharArray &padding)
	{
		CharArray result = *this;

		while(result.size() < length)
		{
			size_t remaining = length - size();
			result += padding.copy(0, remaining);
		}

		return result;
	}
	
	CharArray CharArray::rjust(size_t length, const CharArray &padding)
	{
		CharArray result;

		while(result.size() < length - size())
		{
			size_t remaining = length - size();
			result += padding.copy(0, remaining);
		}

		result += *this;

		return result;
	}

	CharArray CharArray::trim(size_t cut, CharArray append)
	{
		if(length <= cut)
			return *this;

		return copy(0, cut) + append;
	}
	
	void CharArray::localize()
	{
		if(shared)
		{
			char_t *new_data = (char_t *)Collector::allocate(length);

			memcpy(new_data, data, length);

			data = new_data;
			shared = false;
			static_data = false;
		}
	}
	
	bool CharArray::downcase()
	{
		bool changed = false;

		localize();

		for(size_t i = 0; i < length; ++i)
			if(data[i] >= 'A' && data[i] <= 'Z')
			{
				changed = true;
				data[i] = 'a' + (data[i] - 'A');
			}

		return changed;
	}
	
	bool CharArray::upcase()
	{
		bool changed = false;

		localize();

		for(size_t i = 0; i < length; ++i)
			if(data[i] >= 'a' && data[i] <= 'z')
			{
				changed = true;
				data[i] = 'A' + (data[i] - 'a');
			}

		return changed;
	}

	void CharArray::append(const CharArray& other)
	{
		// Note: other may be equal to *this

		char_t *this_data = data;
		char_t *other_data = other.data;

		if(shared)
		{
			data = (char_t *)Collector::allocate(length + other.length);
			memcpy(data, this_data, length);
		}
		else
			data = (char_t *)Collector::reallocate(data, length, length + other.length);
		
		shared = false;
		static_data = false;

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

	char_t *const &CharArray::str_ref() const
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
	
	CharArray operator+(const char_t *lhs, const CharArray &rhs)
	{
		CharArray char_array((const char_t *)lhs);
		char_array.append(rhs);
		return char_array;
	}

	CharArray operator+(const CharArray &lhs, const char_t *rhs)
	{
		CharArray char_array(lhs);
		char_array.append(CharArray((const char_t *)rhs));
		return char_array;
	}
	
	const char_t &CharArray::operator [](size_t index) const
	{
		mirb_debug_assert(index < length);
		return data[index];
	}

	char_t &CharArray::operator [](size_t index)
	{
		mirb_debug_assert(index < length);
		return data[index];
	}

	CharArray CharArray::copy(size_t offset, size_t size) const
	{
		if(offset >= length)
			return CharArray("");

		if((size > length) || (offset + size > length))
			return CharArray(data + offset, length - offset);

		return CharArray(data + offset, size);
	}
	
	void CharArray::shrink(size_t new_size)
	{
		mirb_debug_assert(new_size <= length);
		length = new_size;
	}

	bool CharArray::equal(size_t offset, size_t other_offset, const CharArray &other, size_t size) const
	{
		if(offset + size > length || other_offset + size > other.length)
			return false;

		return std::memcmp(data + offset, other.data + other_offset, size) == 0;
	}

	String *CharArray::to_string() const
	{
		auto result = Collector::allocate<String>(*this);

		Value::assert_valid(result);

		return result;
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
