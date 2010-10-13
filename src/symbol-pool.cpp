#include "range.hpp"
#include "symbol-pool.hpp"

namespace Mirb
{
	SymbolPool symbol_pool;

	size_t SymbolPoolFunctions::hash_key(StringKey &key)
	{
		size_t hash = 0;
		const char_t *start = key.c_str;
		const char_t *stop = start + key.length;

		for(const char_t *c = start; c < stop; c++)
		{
			hash += *c;
			hash += hash << 10;
			hash ^= hash >> 6;
		}

		hash += hash << 3;
		hash ^= hash >> 11;
		hash += hash << 15;

		return hash;
	}
			
	Symbol *SymbolPool::get(Range *range)
	{
		StringKey key;

		key.c_str = range->start;
		key.length = range->stop - key.c_str;

		return SymbolPoolHashTable::get(key);
	};
};
