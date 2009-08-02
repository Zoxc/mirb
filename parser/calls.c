#include "calls.h"
#include "../runtime/symbol.h"

struct node *parse_self_call(struct parser *parser, rt_value symbol)
{
	struct node *result = alloc_node(N_CALL);

	result->left = alloc_node(N_SELF);
	result->middle = (void *)symbol;
	result->right = 0;

	if (is_expression(parser))
		result->right = parse_argument(parser);
	else if (parser_current(parser) == T_PARAM_OPEN)
	{
		next(parser);

		if(parser_current(parser) != T_PARAM_CLOSE)
			result->right = parse_argument(parser);

		match(parser, T_PARAM_CLOSE);
	}

	return result;
}

struct node *parse_call(struct parser *parser, struct node *child)
{
	struct node *result = alloc_node(N_CALL);

	if (require(parser, T_IDENT))
	{
		result->middle = (void *)rt_symbol_from_parser(parser);

		next(parser);
	}
	else
		result->middle = 0;

	result->left = child;

	result->right = 0;

	if (is_expression(parser))
		result->right = parse_argument(parser);
	else if (parser_current(parser) == T_PARAM_OPEN)
	{
		next(parser);

		if(parser_current(parser) != T_PARAM_CLOSE)
			result->right = parse_argument(parser);

		match(parser, T_PARAM_CLOSE);
	}

	return result;
}

static inline bool is_lookup(struct parser *parser)
{
	return parser_current(parser) == T_DOT || parser_current(parser) == T_SQUARE_OPEN;
}

struct node *parse_lookup(struct parser *parser, struct node *child)
{
	switch(parser_current(parser))
	{
		case T_DOT:
			{
				next(parser);

				return parse_call(parser, child);
			}

		case T_SQUARE_OPEN:
			{
				next(parser);

				match(parser, T_SQUARE_CLOSE);

				return 0;
			}

		default:
			assert(0);
	}
}

struct node *parse_lookup_tail(struct parser *parser, struct node *tail)
{
	if(tail && tail->middle)
		return tail;

	switch (parser_current(parser))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
		case T_ASSIGN:
			{
				struct node *result = alloc_node(N_ASSIGN_MESSAGE);

				result->right = tail;
				result->op = parser_current(parser);

				next(parser);

				result->left = parse_expression(parser);

				return result;
			}
			break;

		default:
			return tail;
	}
}

struct node *parse_lookup_chain(struct parser *parser)
{
 	struct node *result = parse_factor(parser);

    while (is_lookup(parser))
    {
    	struct node *node = parse_lookup(parser, result);

		result = node;
    }

	return result;
}

