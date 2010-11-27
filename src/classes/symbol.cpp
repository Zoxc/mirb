#include "symbol.hpp"
#include "../symbol-pool.hpp"

namespace Mirb
{
	value_t Symbol::class_ref;
	
	Symbol *Symbol::from_string(const char *string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_string(std::string string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_char_array(CharArray &char_array)
	{
		return symbol_pool.get(char_array);
	}
};

