#include "symbol.hpp"
#include "string.hpp"
#include "../symbol-pool.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Symbol::class_ref;
	
	Symbol *Symbol::from_string(const char *string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_string(const std::string &string)
	{
		return symbol_pool.get(string);
	}
	
	Symbol *Symbol::from_char_array(CharArray &char_array)
	{
		return symbol_pool.get(char_array);
	}
	
	Symbol *Symbol::from_char_array(CharArray &&char_array)
	{
		CharArray &string = char_array;

		return symbol_pool.get(string);
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
		static_method<Arg::Self>(Symbol::class_ref, "to_s", &to_s);
		static_method<Arg::Self>(Symbol::class_ref, "inspect", &inspect);
	}
};

