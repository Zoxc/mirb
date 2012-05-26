#include "symbol.hpp"
#include "string.hpp"
#include "../symbol-pool.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	Symbol *Symbol::from_cstr_string(const char *string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_string(const std::string &string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::get(const CharArray &char_array)
	{
		return symbol_pool.get(char_array);
	}
	
	Symbol *Symbol::create_initial(const CharArray &char_array)
	{
		Symbol *result = new Symbol();

		result->string = char_array;
		
		result->hash_value = char_array.hash();
		result->hashed = true;

		symbol_pool_list.append(result);

		symbol_pool.set(char_array, result);

		return result;
	}
	
	value_t Symbol::to_s(Symbol *self)
	{
		return self->string.to_string();
	}
	
	value_t Symbol::inspect(Symbol *self)
	{
		CharArray string = ":" + self->string;

		return string.to_string();
	}
	
	value_t to_sym(value_t obj)
	{
		return obj;
	}

	void Symbol::initialize()
	{
		method<Arg::Self<Arg::Class<Symbol>>>(context->symbol_class, "to_s", &to_s);
		method<Arg::Self<Arg::Value>>(context->symbol_class, "to_sym", &to_sym);
		method<Arg::Self<Arg::Class<Symbol>>>(context->symbol_class, "inspect", &inspect);
	}
};

