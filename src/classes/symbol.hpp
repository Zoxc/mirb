#pragma once
#include "../common.hpp"
#include "../value.hpp"
#include "../hash-table.hpp"
#include "../gc.hpp"

#include "../../runtime/classes/symbol.hpp"

namespace Mirb
{
	class Range;

	class StringKey
	{
		public:
			const char_t *c_str;
			size_t length;

			bool operator ==(StringKey &other)
			{
				return length == other.length && memcmp(c_str, other.c_str, length) == 0;
			}
	};

	class Symbol:
		public rt_symbol
	{
		public:
			StringKey string;

			Symbol *next;

			std::string get_string()
			{
				if(this == 0)
				{
					return "<null>";
				}
				else
				{
					std::string result((const char *)string.c_str, string.length);

					return result;
				}
			}

			static value_t class_ref;
	};
};
