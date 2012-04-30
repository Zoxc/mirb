#pragma once
#include "class.hpp"
#include "../common.hpp"
#include <Prelude/LinkedList.hpp>
#include "../value.hpp"
#include "../char-array.hpp"

namespace Mirb
{
	class Range;

	class Symbol:
		public Object
	{
		private:
			static value_t to_s(value_t obj);
			static value_t inspect(value_t obj);

		public:
			Symbol(const CharArray &char_array, size_t hash) : Object(Value::Symbol, context->symbol_class), string(char_array)
			{
				mirb_debug_assert(hash == char_array.hash());

				hash_value = hash;
				hashed = true;
			}

			CharArray string;

			Symbol *next;

			LinkedListEntry<Symbol> entry;

			std::string get_string()
			{
				if(this == 0)
					return "<null>";
				else
					return string.get_string();
			}

			static Symbol *from_string(const char *string);
			static Symbol *from_string(const std::string &string);
			static Symbol *from_char_array(const CharArray &char_array);
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(string);

				if(next)
					mark(next);
			}

			template<size_t length> static Symbol *from_literal(const char (&string)[length])
			{
				CharArray char_array(string);

				return from_char_array(char_array);
			}

			static void initialize();
	};
	
	template<size_t length> Symbol *symbol_cast(const char (&string)[length])
	{
		return Symbol::from_literal(string);
	}
	
	static inline Symbol *symbol_cast(Symbol *symbol)
	{
		return symbol;
	}
};
