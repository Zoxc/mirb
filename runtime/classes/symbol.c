#include "../../globals.h"
#include "symbol.h"
#include "string.h"

HASH_RUNTIME_ROOT_STR(symbol, rt_value);

rt_value rt_Symbol;

static hash_t(symbol) *symbols;

void rt_symbols_create(void)
{
	symbols = hash_init(symbol);
}

void rt_symbols_destroy(void)
{
	hash_destroy(symbol, symbols);
}

rt_value rt_symbol_from_cstr(const char* name)
{
	hash_iter_t i = hash_get(symbol, symbols, name);

	if (i == hash_end(symbols))
	{
		size_t name_length = strlen(name);
		char* string = (char *)rt_alloc_data(name_length + 1);

		strcpy(string, name);

		rt_value symbol = rt_alloc(sizeof(struct rt_symbol));

		RT_COMMON(symbol)->flags = C_SYMBOL;
		RT_COMMON(symbol)->class_of = rt_Symbol;
		RT_COMMON(symbol)->vars = 0;
		RT_SYMBOL(symbol)->string = string;

		int ret;

		i = hash_put(symbol, symbols, string, &ret);

		if(!ret)
		{
			hash_del(symbol, symbols, i);

			printf("Unable to add symbol %s to the list\n", string);
		}

		hash_value(symbols, i) = symbol;

		return symbol;
	}

	return hash_value(symbols, i);
}


/*
 * Symbol
 */

rt_compiled_block(rt_symbol_inspect)
{
	rt_value result = rt_string_from_cstr(":");
	rt_concat_string(result, rt_string_from_cstr(RT_SYMBOL(obj)->string));

	return result;
}

rt_compiled_block(rt_symbol_to_s)
{
	return rt_string_from_cstr(RT_SYMBOL(obj)->string);
}

void rt_symbol_init(void)
{
	rt_define_method(rt_Symbol, "to_s", rt_symbol_to_s);
	rt_define_method(rt_Symbol, "inspect", rt_symbol_inspect);
}
