#include "structures.h"
#include "../runtime/classes/symbol.h"

struct node *parse_class(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(N_CLASS);

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

	struct node *result = alloc_node(N_MODULE);

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

struct node *parse_parameter(struct parser *parser, scope_t *scope)
{
	if(is_parameter(parser))
	{
		struct node *result = alloc_node(N_PARAMETER);

		result->middle = (void *)matches(parser, T_AMP);

		result->left = (void *)rt_symbol_from_parser(parser);

		if(result->middle)
		{
			if(scope->block_var)
			{
				parser->err_count++;
				printf("You can only receive the block in one parameter.\n");
			}
			else
			{
				scope->block_var = scope_define(scope, (rt_value)result->left, V_TEMP, false);
			}
		}
		else
		{
			if(scope_defined(scope, (rt_value)result->left, false))
			{
				parser->err_count++;
				printf("Parameter %s already defined.\n", rt_symbol_to_cstr((rt_value)result->left));
			}
			else
				scope_define(scope, (rt_value)result->left, V_PARAMETER, false);
		}

		next(parser);

		if(matches(parser, T_COMMA))
			result->right = parse_parameter(parser, scope);
		else
			result->right = 0;

		return result;
	}
	else
	{
		parser->err_count++;
		printf("Expected paramater, but found %s.\n", token_type_names[parser_current(parser)]);

		return 0;
	}
}

struct node *parse_method(struct parser *parser)
{
	parser_state(parser, TS_NOKEYWORDS);

	next(parser);

	parser_state(parser, TS_DEFAULT);

	struct node *result = alloc_node(N_METHOD);

	if(require(parser, T_IDENT))
	{
		result->left = (void *)rt_symbol_from_parser(parser);

		next(parser);
	}
	else
		result->left = 0;

	scope_t *scope;

	result->right = alloc_scope(parser, &scope, S_METHOD);

	if(matches(parser, T_PARAM_OPEN))
	{
		result->middle = parse_parameter(parser, scope);

		match(parser, T_PARAM_CLOSE);
	}
	else
	{
		if(is_parameter(parser))
			result->middle = parse_parameter(parser, scope);
		else
			result->middle = 0;

		parse_sep(parser);
	}

	result->right->right = parse_statements(parser);

	parser->current_scope = scope->owner;

	match(parser, T_END);

	return result;
}
