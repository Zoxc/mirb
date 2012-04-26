#include "generic/range.hpp"
#include "symbol-pool.hpp"

namespace Mirb
{
	SymbolPool symbol_pool;

	Symbol *SymbolPool::get(Range &range)
	{
		CharArray key(range.start, range.stop - range.start);

		return SymbolPoolHashTable::get(key);
	};

	void fix_symbol_pool()
	{
		symbol_pool.each_value([&](Symbol *symbol) {
			symbol->instance_of = auto_cast(context->symbol_class);
		});
	}
};
