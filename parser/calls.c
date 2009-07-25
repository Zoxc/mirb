#include "calls.h"
#include "../runtime/symbols.h"


struct node *parse_message(struct parser *parser, struct node **tail, rt_value symbol)
{
	if (symbol || parser_current(parser) == T_DOT)
	{
		struct node *result = alloc_node(N_MESSAGE);

		*tail = result;

		if (symbol)
		{
			result->left = (void *)symbol;
		}
		else
		{
			next(parser);

			if (require(parser, T_IDENT))
			{
				result->left = (void *)symbol_from_parser(parser);
				next(parser);
			}
			else
				result->left = 0;
		}

		result->middle = 0;

		if (is_expression(parser))
			result->middle = parse_argument(parser);
		else if (parser_current(parser) == T_PARAM_OPEN)
		{
			next(parser);

			if(parser_current(parser) != T_PARAM_CLOSE)
				result->middle = parse_argument(parser);

			match(parser, T_PARAM_CLOSE);
		}

		switch (parser_current(parser))
		{
			case T_DOT:
			case T_SQUARE_OPEN:
				result->right = parse_message(parser, tail, 0);
				break;

			default:
				result->right = 0;
		}

		return result;
	}
	else if (parser_current(parser) == T_SQUARE_OPEN)
	{
		next(parser);

		struct node *result = alloc_node(N_ARRAY_MESSAGE);

		result->left = parse_expression(parser);

		match(parser, T_SQUARE_CLOSE);

		switch (parser_current(parser))
		{
			case T_DOT:
			case T_SQUARE_OPEN:
				result->right = parse_message(parser, tail, 0);
				break;

			default:
				result->right = 0;
		}

		return result;
	}

	return 0;
}

void parse_call_tail(struct parser *parser, struct node *tail)
{
	if(tail && tail->middle)
		return;

	switch (parser_current(parser))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
		case T_ASSIGN:
			{
				struct node *result = alloc_node(N_CALL_TAIL);

				tail->right = result;

				result->op = parser_current(parser);

				next(parser);

				result->left = parse_expression(parser);
			}
			break;

		default:
			break;
	}
}

struct node *parse_call(struct parser *parser, struct node *object)
{
	struct node *call = alloc_node(N_CALL);
	struct node *tail;

	call->left = object;
	call->right = parse_message(parser, &tail, 0);

	if(call->right == 0)
	{
    	parser->err_count++;

        printf("Excepted message but found %s\n", token_type_names[parser_current(parser)]);
	}

	parse_call_tail(parser, tail);

	return call;
}
