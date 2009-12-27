#pragma once
#include "../classes.h"
#include "../../compiler/lexer.h"
#include "../../compiler/parser/parser.h"
#include "string.h"

struct rt_symbol {
	struct rt_common common;
	const char* string;
};

#define RT_SYMBOL(value) ((struct rt_symbol *)(value))

extern rt_value rt_Symbol;

void rt_symbols_create(void);
void rt_symbols_destroy(void);

rt_value rt_symbol_from_cstr(const char *name);

static inline rt_value rt_symbol_from_token(struct token *token)
{
	char *str = get_token_str(token);

	rt_value result = rt_symbol_from_cstr(str);

	return result;
}

static inline const char *rt_symbol_to_cstr(rt_value value)
{
	return RT_SYMBOL(value)->string;
}

static inline rt_value rt_symbol_from_string(rt_value value)
{
	return rt_string_from_cstr(rt_symbol_to_cstr(value));
}

static inline rt_value rt_string_from_symbol(rt_value value)
{
	return rt_string_from_cstr(RT_SYMBOL(value)->string);
}

static inline bool rt_symbol_is_const(rt_value value)
{
	const char c = *rt_symbol_to_cstr(value);

	return c >= 'A' && c <= 'Z';
}

static inline rt_value rt_symbol_from_lexer(struct compiler *compiler)
{
	return rt_symbol_from_token(lexer_token(compiler));
}

void rt_symbol_init(void);

rt_compiled_block(rt_symbol_to_s);
