#include "symbols.h"
#include "globals.h"

KHASH_MAP_INIT_STR(str, void*);

static khash_t(str) *symbol_list;

void symbols_create()
{
	symbol_list = kh_init(str);
}

void symbols_destroy()
{
	kh_destroy(str, symbol_list);
}

void *symbol_get(char* name)
{
	khiter_t k = kh_get(str, symbol_list, name);

	if (k == kh_end(symbol_list))
	{
		unsigned int name_length = strlen(name);
		char *symbol = malloc(name_length + 1);
		strcpy(symbol, name);

		int ret;

		k = kh_put(str, symbol_list, (void*)symbol, &ret);

		if(!ret)
		{
			kh_del(str, symbol_list, k);

			printf("Unable to add symbol %s to the list\n", symbol);
		}

		kh_value(symbol_list, k) = symbol;

		return symbol;
	}

	return kh_value(symbol_list, k);
}

