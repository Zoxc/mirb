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
		else
			result->right = (void *)1;

		match(parser, T_PARAM_CLOSE);
	}

	return result;
}

static inline bool is_lookup(struct parser *parser)
{
	return parser_current(parser) == T_DOT || parser_current(parser) == T_SQUARE_OPEN || parser_current(parser) == T_SCOPE;
}

struct node *parse_lookup(struct parser *parser, struct node *child)
{
	switch(parser_current(parser))
	{
		case T_SCOPE:
			{
				next(parser);

				if(require(parser, T_IDENT))
				{
					rt_value symbol = rt_symbol_from_parser(parser);
					struct node *result;

					if(rt_symbol_is_const(symbol))
					{
						result = alloc_node(N_CONST);
						result->left = child;
						result->right = (void *)symbol;
					}
					else
					{
						result = alloc_node(N_CALL);
						result->left = child;
						result->middle = (void *)symbol;
						result->right = 0;
					}

					next(parser);

					return result;
				}
				else
					return child;
			}

		case T_DOT:
			{
				next(parser);

				return parse_call(parser, child);
			}

		case T_SQUARE_OPEN:
			{
				next(parser);

				match(parser, T_SQUARE_CLOSE);

				return child;
			}

		default:
			assert(0);
	}
}

struct node *parse_lookup_tail(struct parser *parser, struct node *tail)
{
	if(tail && tail->type == N_CALL && tail->right)
		return tail;

	switch(parser_current(parser))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
			assert(0);

		case T_ASSIGN:
			{
				switch(tail->type)
				{
					case N_CONST:
						{
							struct node *result = alloc_node(N_ASSIGN_CONST);

							result->left = tail->left;
							result->middle = tail->right;

							free(tail);

							next(parser);

							result->right = parse_expression(parser);

							return result;
						}

					default:
						assert(0);
				}

				struct node *result = alloc_node(N_ASSIGN_CALL);

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

    while(is_lookup(parser))
    {
    	struct node *node = parse_lookup(parser, result);

		result = node;
    }

	return parse_lookup_tail(parser, result);
}

