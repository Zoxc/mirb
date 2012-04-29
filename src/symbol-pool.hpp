#pragma once
#include "common.hpp"
#include <Prelude/HashTable.hpp>
#include "collector.hpp"
#include "classes/symbol.hpp"

namespace Mirb
{
	extern LinkedList<Symbol> symbol_pool_list;

	class SymbolPoolFunctions:
		public HashTableFunctions<const CharArray &, Symbol *>
	{
		public:
			static bool compare_key_value(const CharArray &key, Symbol *value)
			{
				return key == value->string;
			}

			static CharArray &get_key(Symbol *value)
			{
				return value->string;
			}

			static Symbol *get_value_next(Symbol *value)
			{
				return value->next;
			}

			static void set_value_next(Symbol *value, Symbol *next)
			{
				value->next = next;
			}
			
			static size_t hash_key(const CharArray &key)
			{
				return key.hash();
			}
			
			static bool valid_key(const CharArray &)
			{
				return true;
			}

			static bool create_value()
			{
				return true;
			}

			static Symbol *create_value(Prelude::Allocator::Standard::Reference, const CharArray &key)
			{
				Symbol *result = new Symbol(key); // TODO: Allocate as root memory.

				symbol_pool_list.append(result);

				return result;
			}
			
			static void free_value(Prelude::Allocator::Standard::Reference, Symbol *value)
			{
				symbol_pool_list.remove(value);

				delete value;
			}
			
			template<typename F> static void mark_value(Symbol *&value, F mark)
			{
				mark(value);
			}
	};

	typedef HashTable<const CharArray &, Symbol *, SymbolPoolFunctions> SymbolPoolHashTable;

	class SymbolPool:
		public SymbolPoolHashTable
	{
		public:
			SymbolPool() : SymbolPoolHashTable(8) {}
			
			Symbol *get(const CharArray &char_array)
			{
				return SymbolPoolHashTable::get(char_array);
			}

			Symbol *get(const char *string)
			{
				CharArray key((const char_t *)string);

				return SymbolPoolHashTable::get(key);
			}

			Symbol *get(const std::string &string)
			{
				CharArray key(string);

				return SymbolPoolHashTable::get(key);
			}

			Symbol *get(const char_t *string, size_t length)
			{
				CharArray key(string, length);

				return SymbolPoolHashTable::get(key);
			}

			Symbol *get(Range &range);

			Symbol *get(const char_t *start, const char_t *stop)
			{
				CharArray key(start, stop - start);

				return SymbolPoolHashTable::get(key);
			}
	};
	
	extern SymbolPool symbol_pool;

	void fix_symbol_pool();
};
