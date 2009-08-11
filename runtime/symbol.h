#pragma once
#include "classes.h"
#include "../parser/lexer.h"
#include "../parser/parser.h"

struct rt_symbol {
	struct rt_common common;
	const char* string;
};

#define RT_SYMBOL(value) ((struct rt_symbol *)value)

extern rt_value rt_Symbol;

void rt_symbols_create(void);
void rt_symbols_destroy(void);

rt_value rt_symbol_from_cstr(const char *name);

static inline rt_value rt_symbol_from_token(token_t *token)
{
	char *str = get_token_str(token);

	rt_value result = rt_symbol_from_cstr(str);

	free(str);

	return result;
}

static inline const char *rt_symbol_to_cstr(rt_value value)
{
	return RT_SYMBOL(value)->string;
}

static inline bool rt_symbol_is_const(rt_value value)
{
	const char c = *rt_symbol_to_cstr(value);

	return c >= 'A' && c <= 'Z';
}

static inline rt_value rt_symbol_from_parser(struct parser *parser)
{
	return rt_symbol_from_token(parser_token(parser));
}

void rt_symbol_init(void);

rt_value rt_symbol_to_s(rt_value obj, unsigned int argc);
