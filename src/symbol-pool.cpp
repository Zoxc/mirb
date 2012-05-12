#include "generic/source-loc.hpp"
#include "symbol-pool.hpp"

namespace Mirb
{
	SymbolPool symbol_pool;
	LinkedList<Symbol> symbol_pool_list;

	Symbol *SymbolPool::get(SourceLoc &range)
	{
		CharArray key(range.start, range.stop - range.start);

		return SymbolPoolHashTable::get(key);
	};
};
