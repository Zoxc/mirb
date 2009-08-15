#include "structures.h"
#include "../runtime/symbol.h"

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

struct node *parse_parameter(struct parser *parser, scope_t *scope)
{
	struct node *result = alloc_node(N_PARAMETER);

	if(require(parser, T_IDENT))
	{
		result->left = (void *)rt_symbol_from_parser(parser);

		if(scope_defined(scope, (rt_value)result->left, false))
		{
			parser->err_count++;
			printf("Parameter %s already defined.\n", rt_symbol_to_cstr((rt_value)result->left));
		}
		else
			scope_define(scope, (rt_value)result->left, V_PARAMETER);

		next(parser);

		if(matches(parser, T_COMMA))
			result->right = parse_parameter(parser, scope);
		else
			result->right = 0;
	}
	else
	{
		result->left = 0;
		result->right = 0;
	}

	return result;
}

struct node *parse_method(struct parser *parser)
{
	next(parser);

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
		if(parser_current(parser) == T_IDENT)
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
