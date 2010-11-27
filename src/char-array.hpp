#pragma once
#include "common.hpp"
#include "value.hpp"

namespace Mirb
{
	class CharArray
	{
		public:
			CharArray() : length(0), shared(false) {}
			CharArray(const char_t *c_str);
			CharArray(const std::string &string);
			CharArray(const char_t *c_str, size_t length);
			CharArray(CharArray &char_array);
			CharArray(CharArray &&char_array);
			
			template<size_t length> CharArray(const char (&string)[length])
			{
				set_literal(string);
			}
			
			CharArray& operator=(const char_t *c_str);
			CharArray& operator=(const std::string &string);
			CharArray& operator=(CharArray &other);
			CharArray& operator=(CharArray &&other);
			
			void localize();
			void append(CharArray &other);
			void append(CharArray &&other);
			
			template<size_t length> void append(const char (&string)[length])
			{
				CharArray char_array(string);
				append(char_array);
			}
			
			CharArray &operator+=(CharArray& other);

			size_t hash();
			
			size_t length;
			char_t *data;
			bool shared;

			value_t to_string();
			
			template<size_t length> void set_literal(const char (&string)[length])
			{
				data = (char_t *)&string;
				this->length = length;
				shared = true;
			}
			
			bool operator ==(CharArray &other)
			{
				return length == other.length && memcmp(data, other.data, length) == 0;
			}

			std::string get_string()
			{
				return std::string((const char *)data, length);
			}
			
			static CharArray hex(size_t value);
	};
	
	template<size_t length> CharArray operator+(const char (&lhs)[length], CharArray &rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}
	
	template<size_t length> CharArray operator+(const char (&lhs)[length], CharArray &&rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}
	
	template<size_t length> CharArray operator+(CharArray &lhs, const char (&rhs)[length])
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}
	
	template<size_t length> CharArray operator+(CharArray &&lhs, const char (&rhs)[length])
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}
	
	CharArray operator+(CharArray &lhs, CharArray &rhs);
	CharArray operator+(CharArray &&lhs, CharArray &rhs);
	CharArray operator+(CharArray &lhs, CharArray &&rhs);
	CharArray operator+(CharArray &&lhs, CharArray &&rhs);
};
