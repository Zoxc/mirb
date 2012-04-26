#pragma once
#include "common.hpp"
#include "value.hpp"

namespace Mirb
{
	class CharArray
	{
		private:
			size_t length;
			char_t *data;
			mutable bool shared;
			bool static_data;
			
			friend class Collector;
			friend struct ThreadFunc;
			template<bool> friend struct OnStackBlockBase;
		public:
			CharArray() : length(0), data(0), shared(false) {}
			CharArray(const char_t *c_str);
			CharArray(const std::string &string);
			CharArray(const char_t *c_str, size_t length);
			CharArray(const CharArray &char_array);
			CharArray(CharArray &&char_array);
			
			template<size_t length> CharArray(const char (&string)[length])
			{
				set_literal(string);
			}
			
			CharArray& operator=(const char_t *c_str);
			CharArray& operator=(const std::string &string);
			CharArray& operator=(const CharArray &other);
			CharArray& operator=(CharArray &&other);
			
			template<size_t length> CharArray &operator=(const char (&string)[length])
			{
				set_literal(string);

				return *this;
			}
			
			void localize();
			void append(const CharArray &other);
			
			template<size_t length> void append(const char (&string)[length])
			{
				CharArray char_array(string);
				append(char_array);
			}
			
			CharArray &operator+=(const CharArray &other);
			
			template<size_t length> CharArray &operator+=(const char (&string)[length])
			{
				append(string);

				return *this;
			}
			
			size_t size() const;
			const char_t *raw() const;
			
			size_t hash() const;
			
			value_t to_string() const;
			
			const char *c_str_ref() const;
			size_t c_str_length() const;

			char_t * const&str_ref() const;
			size_t str_length() const;

			CharArray c_str() const;
			
			template<size_t string_length> void set_literal(const char (&string)[string_length])
			{
				data = (char_t *)&string;
				length = string_length - 1;
				shared = true;
				static_data = true;
			}
			
			bool operator ==(CharArray &other) const
			{
				return length == other.length && memcmp(data, other.data, length) == 0;
			}

			std::string get_string() const
			{
				return std::string((const char *)data, length);
			}
			
			static CharArray hex(size_t value);
			static CharArray uint(size_t value);
	};
	
	CharArray operator *(CharArray lhs, size_t rhs);

	template<size_t length> CharArray operator+(const char (&lhs)[length], const CharArray &rhs)
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}
	
	template<size_t length> CharArray operator+(const CharArray &lhs, const char (&rhs)[length])
	{
		CharArray char_array(lhs);
		char_array.append(rhs);
		return char_array;
	}
	
	CharArray operator+(const CharArray &lhs, const CharArray &rhs);
};
