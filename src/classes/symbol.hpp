#pragma once
#include "../common.hpp"
#include "../value.hpp"
#include "../char-array.hpp"
#include "../generic/hash-table.hpp"
#include "../gc.hpp"

#include "../../runtime/classes/symbol.hpp"

namespace Mirb
{
	class Range;

	class Symbol:
		public rt_symbol
	{
		public:
			CharArray string;

			Symbol *next;

			std::string get_string()
			{
				if(this == 0)
					return "<null>";
				else
					return string.get_string();
			}
			
			static Symbol *from_string(const char *string);
			static Symbol *from_string(std::string string);
			static Symbol *from_char_array(CharArray &char_array);
			
			template<size_t length> static Symbol *from_literal(const char (&string)[length])
			{
				CharArray char_array(string);

				return from_char_array(char_array);
			}

			static value_t class_ref;
	};
};
