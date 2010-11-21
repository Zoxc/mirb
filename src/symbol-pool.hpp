#pragma once
#include "common.hpp"
#include "hash-table.hpp"
#include "gc.hpp"
#include "classes/symbol.hpp"

namespace Mirb
{
	class SymbolPoolFunctions:
		public HashTableFunctions<StringKey &, Symbol *, GC>
	{
		public:
			static bool compare_key_value(StringKey &key, Symbol *value)
			{
				return key == value->string;
			}

			static StringKey &get_key(Symbol *value)
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
			
			static size_t hash_key(StringKey &key);
			
			static bool valid_key(StringKey &key)
			{
				return true;
			}

			static bool create_value()
			{
				return true;
			}

			static Symbol *create_value(GC::Ref alloc_ref, StringKey &key)
			{
				size_t length = key.length;
				Symbol *result = (Symbol *)rt_alloc_root(sizeof(Symbol));

				rt_symbol_setup((rt_value)result);

				result->next = 0;
				result->string.length = key.length;

				char_t *result_str = (char_t *)rt_alloc_data(length + 1);
				memcpy(result_str, key.c_str, length);
				result_str[length] = 0;

				result->string.c_str = result_str;

				return result;
			}
	};

	typedef HashTable<StringKey &, Symbol *, SymbolPoolFunctions, GC> SymbolPoolHashTable;

	class SymbolPool:
		public SymbolPoolHashTable
	{
		public:
			SymbolPool() : SymbolPoolHashTable(8) {}
			
			Symbol *get(const char *string)
			{
				StringKey key;

				key.c_str = (const char_t *)string;
				key.length = strlen(string);

				return SymbolPoolHashTable::get(key);
			}

			Symbol *get(const std::string &string)
			{
				StringKey key;

				key.c_str = (const char_t *)string.c_str();
				key.length = string.length();

				return SymbolPoolHashTable::get(key);
			}

			Symbol *get(const char_t *string, size_t length)
			{
				StringKey key;

				key.c_str = string;
				key.length = length;

				return SymbolPoolHashTable::get(key);
			}

			Symbol *get(Range &range);

			Symbol *get(const char_t *start, const char_t *stop)
			{
				StringKey key;

				key.c_str = start;
				key.length = stop - key.c_str;

				return SymbolPoolHashTable::get(key);
			}
	};

	extern SymbolPool symbol_pool;
};
