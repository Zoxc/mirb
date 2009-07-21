#include "control_blocks.h"

void parse_then_sep(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_THEN:
		case T_COLON:
			lexer_next(lexer);
			break;

		default:
			parse_sep(lexer);
	}
}

struct node *parse_ternary_if(struct lexer* lexer)
{
	struct node *result = parse_arithmetic(lexer);

	if(lexer_current(lexer) == T_QUESTION)
	{
		lexer_next(lexer);

    	struct node *node = alloc_node(N_IF);

		node->left = result;
		node->middle = parse_ternary_if(lexer);

		match(lexer, T_COLON);

		node->right = parse_ternary_if(lexer);

		return node;
	}

	return result;
}

struct node *parse_unless(struct lexer* lexer)
{
				lexer_next(lexer);

				struct node *result = alloc_node(N_UNLESS);
				result->left = parse_expression(lexer);

				parse_then_sep(lexer);

				result->middle = parse_expressions(lexer);
				result->right = alloc_nil_node();

				match(lexer, T_END);

				return result;
}

struct node *parse_if(struct lexer* lexer)
{
				lexer_next(lexer);

				struct node *result = alloc_node(N_IF);
				result->left = parse_expression(lexer);

				parse_then_sep(lexer);

				result->middle = parse_expressions(lexer);

				if(lexer_current(lexer) == T_ELSE)
				{
					lexer_next(lexer);

					result->right = parse_expressions(lexer);
				}
				else
					result->right = alloc_nil_node();

				match(lexer, T_END);

				return result;
}
