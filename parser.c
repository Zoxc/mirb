#include "parser.h"
#include "symbols.h"

inline bool match(struct lexer* lexer, token_type type)
{
    if (lexer_current(lexer) == type)
    {
        lexer_next(lexer);

        return true;
    }
    else
    {
    	(lexer->err_count)++;
        printf("Excepted token %s but found %s\n", token_type_names[type], token_type_names[lexer_current(lexer)]);

        return false;
    }
}

static inline struct node *alloc_node(node_type type)
{
	struct node *result = malloc(sizeof(struct node));
	result->type = type;

	return result;
}

struct node *parse_call_tail(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
			{
				struct node *result = alloc_node(N_CALL_TAIL);

				result->op = lexer_current(lexer);

				lexer_next(lexer);

				result->left = parse_expression(lexer);

				return result;
			}
			break;

		default:
			return 0;
	}
}

struct node *parse_argument(struct lexer* lexer)
{
	printf("N_ARGUMENT ENTER\n");
	printf("N_ARGUMENT %s\n", token_type_names[lexer_current(lexer)]);

	struct node *result = parse_expression(lexer);

	while (lexer_current(lexer) == T_COMMA)
	{
		struct node *node = alloc_node(N_ARGUMENT);
		node->op = lexer_current(lexer);
		node->left = result;

		lexer_next(lexer);

		printf("N_ARGUMENT %s\n", token_type_names[lexer_current(lexer)]);

		node->right = parse_expression(lexer);
		result = node;
	}

	printf("N_ARGUMENT LEAVE\n");

	return result;
}

struct node *parse_message(struct lexer* lexer, void *symbol)
{
	if (symbol || lexer_current(lexer) == T_IDENT)
	{
		int in_args = lexer->in_args;

		lexer->in_args++;

		struct node *result = alloc_node(N_MESSAGE);

		if (symbol)
		{
			result->left = symbol;
		}
		else
		{
			result->left = (void *)get_token_str(lexer_current_token(lexer));
			lexer_next(lexer);
		}

		printf("N_MESSAGE %s %d\n", result->left, in_args);

		result->middle = 0;

		switch (lexer_current(lexer))
		{
			case T_PARAM_OPEN:
				{
					lexer_next(lexer);

					result->middle = parse_argument(lexer);

					match(lexer, T_PARAM_CLOSE);
				}
				break;

			case T_IDENT:
			case T_ADD:
			case T_SUB:
			case T_NUMBER:
				{
					if(in_args)
						printf("N_MESSAGE NO_ARGS\n");
					else
					{
						printf("N_MESSAGE ARGS\n");
						result->middle = parse_argument(lexer);
					}
				}
				break;

			default:
				break;
		}

		if (lexer_current(lexer) == T_DOT)
		{
			lexer_next(lexer);

			result->right = parse_message(lexer, 0);
		}

		lexer->in_args--;

		return result;
	}

	return 0;
}

struct node *parse_factor(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_NUMBER:
			{
				struct node *result = alloc_node(N_NUMBER);

				result->left = (void*)get_token_str(lexer->token);
				result->right = (void*)atoi((char*)result->left);

				lexer_next(lexer);

				return result;
			}

		case T_IDENT:
			{
				char* name = get_token_str(lexer_current_token(lexer));
				void* symbol = symbol_get(name);
				free(name);

				lexer_next(lexer);

				switch(lexer_current(lexer))
				{
					case T_ASSIGN_ADD:
					case T_ASSIGN_SUB:
					case T_ASSIGN_MUL:
					case T_ASSIGN_DIV:
					{
						struct node *result = alloc_node(N_ASSIGN);

						token_type op_type = lexer_current(lexer) - 4;

						lexer_next(lexer);

						result->left = symbol;
						result->right = alloc_node(N_EXPRESSION);
						result->right->op = op_type;
						result->right->left = alloc_node(N_VAR);
						result->right->left->left = symbol;
						result->right->right = parse_expression(lexer);

						return result;
					}

					case T_ASSIGN:
					{
						struct node *result = alloc_node(N_ASSIGN);

						lexer_next(lexer);

						result->left = symbol;
						result->right = parse_expression(lexer);

						return result;
					}

					// Function call or local variable

					case T_ADD:
					case T_SUB:
					case T_MUL:
					case T_DIV:
					case T_QUESTION:
					case T_COLON:
					case T_LINE:
					case T_EOF:
					{
						struct node *result = alloc_node(N_VAR);

						result->left = symbol;

						return result;
					}

					// Function call
					default:
					{
						struct node *result;

						printf("N_CALL %s\n", symbol);

						if(lexer_current(lexer) == T_DOT)
						{
							lexer_next(lexer);

							result = alloc_node(N_CALL);
							result->left = symbol;
							result->middle = parse_message(lexer, 0);
							result->right = parse_call_tail(lexer);
						}
						else
						{
							result = alloc_node(N_CALL);
							result->left = 0;
							result->middle = parse_message(lexer, symbol);
							result->right = parse_call_tail(lexer);
						}

						return result;
					}
				}
			}

		case T_PARAM_OPEN:
			{
				lexer_next(lexer);

				struct node *result = parse_expression(lexer);

				match(lexer, T_PARAM_CLOSE);

				return result;
			}

		default:
			{
				(lexer->err_count)++;
				printf("Excepted expression but found %s\n", token_type_names[lexer_current(lexer)]);
				lexer_next(lexer);

				return 0;
			}
	}
}

struct node *parse_unary(struct lexer* lexer)
{
    return parse_factor(lexer);
}

struct node *parse_term(struct lexer* lexer)
{
	struct node *result = parse_unary(lexer);

    while (lexer_current(lexer) == T_MUL || lexer_current(lexer) == T_DIV)
    {
    	struct node *node = alloc_node(N_TERM);
		node->op = lexer_current(lexer);
		node->left = result;

		lexer_next(lexer);

		node->right = parse_unary(lexer);
		result = node;
    }

	return result;
}

struct node *parse_arithmetic(struct lexer* lexer)
{
	struct node *result = parse_term(lexer);

    while (lexer_current(lexer) == T_ADD || lexer_current(lexer) == T_SUB)
    {
    	struct node *node = alloc_node(N_EXPRESSION);
		node->op = lexer_current(lexer);
		node->left = result;

		lexer_next(lexer);

		node->right = parse_term(lexer);
		result = node;
    }

    return result;
}

struct node *parse_ternary_if(struct lexer* lexer)
{
	struct node *result = parse_arithmetic(lexer);

	if(lexer_current(lexer) == T_QUESTION)
	{
		lexer_next(lexer);

		// Create a N_IF node
    	struct node *node = alloc_node(N_IF);
		node->op = lexer_current(lexer);
		node->left = result;

		// Create a N_ELSE node
		node->right = alloc_node(N_ELSE);
		node->right->left = parse_ternary_if(lexer);

		match(lexer, T_COLON);

		node->right->right = parse_ternary_if(lexer);

		return node;
	}

	return result;
}

struct node *parse_expression(struct lexer *lexer)
{
	return parse_ternary_if(lexer);
}
