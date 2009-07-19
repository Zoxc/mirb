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

struct node *parse_factor(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_NUMBER:
			{
				struct node *result = alloc_node(N_NUMBER);

				result->left = get_token_str(lexer->token);
				result->right = (void*)atoi(result->left);

				lexer_next(lexer);

				return result;
			}

		case T_IDENT:
			{
				char* name = get_token_str(lexer->token);
				void* symbol = symbol_get(name);
				free(name);

				lexer_next(lexer);

				if (lexer_current(lexer) == T_ASSIGN)
				{
					struct node *result = alloc_node(N_ASSIGN);

					lexer_next(lexer);

					result->left = symbol;
					result->right = parse_expression(lexer);

					return result;
				}
				else
				{
					struct node *result = alloc_node(N_VAR);

					result->left = symbol;

					return result;
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

struct node *parse_expression(struct lexer* lexer)
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

