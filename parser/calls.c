#include "calls.h"


struct node *parse_message(struct lexer* lexer, struct node **tail, void *symbol)
{
	if (symbol || lexer_current(lexer) == T_DOT)
	{
		struct node *result = alloc_node(N_MESSAGE);

		*tail = result;

		if (symbol)
		{
			result->left = symbol;
		}
		else
		{
			lexer_next(lexer);

			if(lexer_current(lexer) != T_IDENT)
			{
				lexer->err_count++;
				printf("Excepted message but found %s\n", token_type_names[lexer_current(lexer)]);
			}
			else
			{
				result->left = (void *)get_token_str(lexer_current_token(lexer));
				lexer_next(lexer);
			}
		}

		result->middle = 0;

		if (is_expression(lexer))
			result->middle = parse_argument(lexer);
		else if (lexer_current(lexer) == T_PARAM_OPEN)
		{
			lexer_next(lexer);

			if(lexer_current(lexer) != T_PARAM_CLOSE)
				result->middle = parse_argument(lexer);

			match(lexer, T_PARAM_CLOSE);
		}

		switch (lexer_current(lexer))
		{
			case T_DOT:
			case T_SQUARE_OPEN:
				result->right = parse_message(lexer, tail, 0);

			default:
				result->right = 0;
		}

		return result;
	}
	else if (lexer_current(lexer) == T_SQUARE_OPEN)
	{
		lexer_next(lexer);

		struct node *result = alloc_node(N_ARRAY_MESSAGE);

		result->left = parse_expression(lexer);

		match(lexer, T_SQUARE_CLOSE);

		switch (lexer_current(lexer))
		{
			case T_DOT:
			case T_SQUARE_OPEN:
				result->right = parse_message(lexer, tail, 0);

			default:
				result->right = 0;
		}

		return result;
	}

	return 0;
}

void parse_call_tail(struct lexer* lexer, struct node *tail)
{
	if(tail && tail->middle)
		return;

	switch (lexer_current(lexer))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
		case T_ASSIGN:
			{
				struct node *result = alloc_node(N_CALL_TAIL);

				tail->right = result;

				result->op = lexer_current(lexer);

				lexer_next(lexer);

				result->left = parse_expression(lexer);
			}
			break;

		default:
			break;
	}
}

struct node *parse_call(struct lexer* lexer, struct node *object)
{
	struct node *call = alloc_node(N_CALL);
	struct node *tail;

	call->left = object;
	call->right = parse_message(lexer, &tail, 0);

	if(call->right == 0)
	{
    	lexer->err_count++;

        printf("Excepted message but found %s\n", token_type_names[lexer_current(lexer)]);
	}

	parse_call_tail(lexer, tail);

	return call;
}
