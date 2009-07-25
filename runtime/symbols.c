#include "symbols.h"
#include "../globals.h"

KHASH_MAP_INIT_STR(symbol, rt_value);

rt_value rt_Symbol_symbol;
static khash_t(symbol) *symbols;

void symbols_create(void)
{
	symbols = kh_init(symbol);

	rt_Symbol_symbol = rt_alloc(sizeof(struct rt_symbol));
	RT_COMMON(rt_Symbol_symbol)->flags = C_SYMBOL;
	RT_SYMBOL(rt_Symbol_symbol)->string = "Symbol";

	int ret;

	khiter_t k = kh_put(symbol, symbols, "Symbol", &ret);

	if(!ret)
	{
		kh_del(symbol, symbols, k);

		printf("Unable to add symbol Symbol to the list\n");
	}

	kh_value(symbols, k) = rt_Symbol_symbol;
}

void symbols_destroy(void)
{
	kh_destroy(symbol, symbols);
}

rt_value symbol_get(const char* name)
{
	khiter_t k = kh_get(symbol, symbols, name);

	if (k == kh_end(symbols))
	{
		unsigned int name_length = strlen(name);
		char* string =  malloc(name_length + 1);

		strcpy(string, name);

		rt_value symbol = rt_alloc(sizeof(struct rt_symbol));

		RT_COMMON(symbol)->flags = C_SYMBOL;
		RT_COMMON(symbol)->class_of = rt_Symbol;
		RT_SYMBOL(symbol)->string = string;

		int ret;

		k = kh_put(symbol, symbols, string, &ret);

		if(!ret)
		{
			kh_del(symbol, symbols, k);

			printf("Unable to add symbol %s to the list\n", string);
		}

		kh_value(symbols, k) = symbol;

		return symbol;
	}

	return kh_value(symbols, k);
}

