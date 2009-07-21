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

struct node *parse_conditional(struct lexer *lexer)
{
	struct node *result = parse_ternary_if(lexer);

    if (lexer_current(lexer) == T_IF || lexer_current(lexer) == T_UNLESS)
    {
     	struct node *node = alloc_node(lexer_current(lexer) == T_IF ? N_IF : N_UNLESS);

		lexer_next(lexer);

		node->middle = result;
		node->left = parse_expression(lexer);
		node->right = alloc_nil_node();

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

				result->middle = parse_statements(lexer);
				result->right = alloc_nil_node();

				match(lexer, T_END);

				return result;
}

struct node *parse_if_tail(struct lexer* lexer)
{
	switch(lexer_current(lexer))
	{
		case T_ELSIF:
			{
				lexer_next(lexer);

				struct node *result = alloc_node(N_IF);
				result->left = parse_expression(lexer);

				parse_then_sep(lexer);

				result->middle = parse_statements(lexer);
				result->right = parse_if_tail(lexer);

				return result;
			}
		case T_ELSE:
			lexer_next(lexer);

			return parse_statements(lexer);

		default:
			return alloc_nil_node();
	}
}

struct node *parse_if(struct lexer* lexer)
{
	lexer_next(lexer);

	struct node *result = alloc_node(N_IF);
	result->left = parse_expression(lexer);

	parse_then_sep(lexer);

	result->middle = parse_statements(lexer);
	result->right = parse_if_tail(lexer);

	match(lexer, T_END);

	return result;
}

struct node *parse_case_body(struct lexer* lexer)
{
	switch(lexer_current(lexer))
	{
		case T_WHEN:
			{
				lexer_next(lexer);

				struct node *result = alloc_node(N_IF);
				result->left = parse_expression(lexer);

				parse_then_sep(lexer);

				result->middle = parse_statements(lexer);
				result->right = parse_case_body(lexer);

				return result;
			}

		case T_ELSE:
			{
				lexer_next(lexer);

				return parse_statements(lexer);
			}

		default:
			{
				lexer->err_count++;
				printf("Excepted else or when but found %s\n", token_type_names[lexer_current(lexer)]);

				return 0;
			}
	}
}

struct node *parse_case(struct lexer* lexer)
{
	lexer_next(lexer);

	struct node *result = 0;

	switch(lexer_current(lexer))
	{
		case T_ELSE:
			lexer_next(lexer);

			result = parse_statements(lexer);
			break;

		case T_WHEN:
			result = parse_case_body(lexer);
			break;

		default:
			lexer->err_count++;
			printf("Excepted else or when but found %s\n", token_type_names[lexer_current(lexer)]);
	}

	match(lexer, T_END);

	return result;
}
