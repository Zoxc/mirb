#pragma once
#include "classes.h"
#include "../parser/lexer.h"
#include "../parser/parser.h"

extern rt_value rt_Symbol_symbol;

void symbols_create(void);
void symbols_destroy(void);
rt_value symbol_get(const char *name);

static inline rt_value symbol_from_token(struct token *token)
{
	char *str = get_token_str(token);

	rt_value result = symbol_get(str);

	free(str);

	return result;
}

static inline rt_value symbol_from_parser(struct parser *parser)
{
	return symbol_from_token(parser_current_token(parser));
}
