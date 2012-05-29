#pragma once
#include "common.hpp"
#include "value.hpp"

namespace Mirb
{
	namespace Accesser
	{
		struct CharArray;
	};

	class CharArray
	{
		private:
			size_t length;
			char_t *data;

			mutable bool shared;
			bool static_data;
			
			friend struct Accesser::CharArray;
		public:
			CharArray() : length(0), data(0), shared(false) {}
			
			CharArray(const char_t c);
			CharArray(const char_t *c_str);
			CharArray(const std::string &string);
			CharArray(const char_t *c_str, size_t length);
			CharArray(const CharArray &char_array);
			CharArray(CharArray &&char_array);
			
			CharArray chomp();
			CharArray trim(size_t cut, CharArray append);

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

			int to_i();
			
			const char_t &operator [](size_t index) const;
			char_t &operator [](size_t index);

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
			
			bool downcase();
			bool upcase();

			CharArray copy(size_t offset, size_t size) const;
			
			bool equal(size_t offset, size_t other_offset, const CharArray &other, size_t size) const;
			
			template<typename F> void each_char(F func)
			{
				for(size_t i = 0; i < length; ++i)
					func((*this)[i], i);
			}
			
			CharArray ljust(size_t length, const CharArray &padding);
			CharArray rjust(size_t length, const CharArray &padding);

			template<typename F> void split(F func, const CharArray &token) const
			{
				size_t last = 0;
				size_t i = 0;

				while(i < length - token.size() + 1)
				{
					if(equal(i, 0, token, token.size()))
					{
						func(copy(last, i - last));

						i += token.size();
						last = i;
					}
					else
						++i;
				}

				if(i != last)
					func(copy(last, i - last));
			}
			
			size_t size() const;
			const char_t *raw() const;
			
			size_t hash() const;
			
			String *to_string() const;
			
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

			void shrink(size_t new_size);
			
			bool operator ==(CharArray &other) const
			{
				return (length == other.length) && (std::memcmp(data, other.data, length) == 0);
			}
			
			template<typename T> bool operator !=(T &&other) const
			{
				return !(*this == other);
			}
			
			template<size_t string_length> bool operator ==(const char (&string)[string_length]) const
			{
				return (string_length - 1 == length) && (std::memcmp(data, &string, string_length - 1) == 0);
			}
			
			std::string get_string() const
			{
				return std::string((const char *)data, length);
			}

			void buffer(size_t size);
			
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
	CharArray operator+(const char_t *lhs, const CharArray &rhs);
	CharArray operator+(const CharArray &lhs, const char_t *rhs);
};
