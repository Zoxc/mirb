#include "symbol.hpp"
#include "string.hpp"
#include "../symbol-pool.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	Symbol *Symbol::from_string(const char *string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_string(const std::string &string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_char_array(const CharArray &char_array)
	{
		return symbol_pool.get(char_array);
	}
	
	value_t Symbol::to_s(value_t obj)
	{
		auto self = cast<Symbol>(obj);

		return self->string.to_string();
	}
	
	value_t Symbol::inspect(value_t obj)
	{
		auto self = cast<Symbol>(obj);

		CharArray string = ":" + self->string;

		return string.to_string();
	}

	void Symbol::initialize()
	{
		method<Arg::Self>(context->symbol_class, "to_s", &to_s);
		method<Arg::Self>(context->symbol_class, "inspect", &inspect);
	}
};

