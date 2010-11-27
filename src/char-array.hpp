#pragma once
#include "common.hpp"

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
			
			CharArray& operator=(const char_t *c_str);
			CharArray& operator=(const std::string &string);
			CharArray& operator=(CharArray& other);

			size_t hash();
			
			size_t length;
			char_t *data;
			bool shared;
			
			bool operator ==(CharArray &other)
			{
				return length == other.length && memcmp(data, other.data, length) == 0;
			}

			std::string get_string()
			{
				return std::string((const char *)data, length);
			}
	};
};
