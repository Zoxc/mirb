#include "structures.h"
#include "../runtime/classes/symbol.h"

struct node *parse_class(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(parser, N_CLASS);

	if(require(parser, T_IDENT))
	{
		result->left = (void *)rt_symbol_from_parser(parser);

		next(parser);
	}
	else
		result->left = 0;

	parse_sep(parser);

	scope_t *scope;

	result->right = alloc_scope(parser, &scope, S_CLASS);
	result->right->right = parse_statements(parser);

	parser->current_scope = scope->owner;

	match(parser, T_END);

	return result;
}

struct node *parse_module(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(parser, N_MODULE);

	if(require(parser, T_IDENT))
	{
		result->left = (void *)rt_symbol_from_parser(parser);

		next(parser);
	}
	else
		result->left = 0;

	parse_sep(parser);

	scope_t *scope;

	result->right = alloc_scope(parser, &scope, S_MODULE);
	result->right->right = parse_statements(parser);

	parser->current_scope = scope->owner;

	match(parser, T_END);

	return result;
}

bool is_parameter(struct parser *parser)
{
	switch(parser_current(parser))
	{
		case T_AMP:
		case T_IDENT:
			return true;

		default:
			return false;
	}
}

void parse_parameter(struct parser *parser, scope_t *scope)
{
	if(is_parameter(parser))
	{
		bool block_var = matches(parser, T_AMP);

		rt_value symbol = rt_symbol_from_parser(parser);

		if(block_var)
		{
			if(scope->block_var)
				PARSER_ERROR(parser, "You can only receive the block in one parameter.");
			else
			{
				scope->block_var = scope_define(scope, symbol, V_BLOCK, false);
			}
		}
		else
		{
			if(scope_defined(scope, symbol, false))
				PARSER_ERROR(parser, "Parameter %s already defined.", rt_symbol_to_cstr(symbol));
			else
				scope_define(scope, symbol, V_PARAMETER, false);
		}

		next(parser);

		if(matches(parser, T_COMMA))
			parse_parameter(parser, scope);
	}
	else
		PARSER_ERROR(parser, "Expected paramater, but found %s.", token_type_names[parser_current(parser)]);
}

struct node *parse_method(struct parser *parser)
{
	parser_state(parser, TS_NOKEYWORDS);

	next(parser);

	parser_state(parser, TS_DEFAULT);

	struct node *result = alloc_node(parser, N_METHOD);

	switch(parser_current(parser))
	{
		case T_IDENT:
		case T_EXT_IDENT:
			{
				result->left = (void *)rt_symbol_from_parser(parser);

				next(parser);
			}
			break;

		default:
			{
				PARSER_ERROR(parser, "Expected identifier but found %s", token_type_names[parser_current(parser)]);

				result->left = 0;
			}
	}

	scope_t *scope;

	result->right = alloc_scope(parser, &scope, S_METHOD);

	if(matches(parser, T_PARAM_OPEN))
	{
		parse_parameter(parser, scope);

		match(parser, T_PARAM_CLOSE);
	}
	else
	{
		if(is_parameter(parser))
			parse_parameter(parser, scope);

		parse_sep(parser);
	}

	result->right->right = parse_statements(parser);

	parser->current_scope = scope->owner;

	match(parser, T_END);

	return result;
}
