#include "string.hpp"
#include "symbol.hpp"

namespace Mirb
{
	value_t String::class_ref;

	value_t String::from_symbol(Symbol *symbol)
	{
		return auto_cast(new (gc) String(symbol->string));
	}
	
	value_t String::from_string(const char *c_str)
	{
		return auto_cast(new (gc) String((const char_t *)c_str, std::strlen(c_str)));
	}
};

