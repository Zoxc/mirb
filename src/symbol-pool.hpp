#pragma once
#include "common.hpp"
#include "hash-table.hpp"

#include "../runtime/classes/symbol.hpp"

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
	};

	class SymbolPoolFunctions:
		public HashTableFunctions<StringKey &, Symbol *, bool>
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

			static Symbol *create_value(bool, StringKey &key)
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

			static Symbol **alloc(bool, size_t entries)
			{
				return (Symbol **)rt_alloc(sizeof(Symbol *) * entries);
			}

			static void free(bool, Symbol **table, size_t entries)
			{
			}
	};

	typedef HashTable<StringKey &, Symbol *, bool, SymbolPoolFunctions> SymbolPoolHashTable;

	class SymbolPool:
		public SymbolPoolHashTable
	{
		public:
			SymbolPool() : SymbolPoolHashTable(8, false) {}
			
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
