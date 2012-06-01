#pragma once
#include "../common.hpp"
#include "class.hpp"
#include <Prelude/LinkedList.hpp>
#include "../value.hpp"
#include "../char-array.hpp"
#include "../context.hpp"

namespace Mirb
{
	class SourceLoc;

	class Symbol:
		public Object
	{
		private:
			static value_t to_s(Symbol *self);
			static value_t inspect(Symbol *self);

			Symbol() : Object(Type::Symbol) {}
			
		public:
			Symbol(const CharArray &char_array, size_t hash) : Object(Type::Symbol, context->symbol_class), string(char_array)
			{
				mirb_debug_assert(hash == char_array.hash());

				hash_value = hash;
			}
			
			size_t hash()
			{
				return hash_value;
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

			static Symbol *from_cstr_string(const char *string);
			static Symbol *from_string(const std::string &string);
			static Symbol *get(const CharArray &char_array);
			static Symbol *create_initial(const CharArray &char_array);
			
			template<typename F> void mark(F mark)
			{
				Object::mark(mark);

				mark(string);

				if(next)
					mark(next);
			}
			
			static void initialize();
	};
	
	template<size_t length> Symbol *symbol_cast(const char (&string)[length])
	{
		return Symbol::get(string);
	}
	
	static inline Symbol *symbol_cast(Symbol *symbol)
	{
		return symbol;
	}
};
