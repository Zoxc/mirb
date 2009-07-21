#include "../symbols.h"
#include "parser.h"
#include "calls.h"
#include "control_blocks.h"

void parse_sep(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_LINE:
		case T_SEP:
			lexer_next(lexer);

		default:
			lexer->err_count++;
			printf("Excepted seperator but found %s\n", token_type_names[lexer_current(lexer)]);
	}
}

struct node *parse_argument(struct lexer* lexer)
{
	struct node *result = parse_expression(lexer);

	while (lexer_current(lexer) == T_COMMA)
	{
		struct node *node = alloc_node(N_ARGUMENT);
		node->left = result;

		lexer_next(lexer);

		node->right = parse_expression(lexer);
		result = node;
	}

	return result;
}

struct node *parse_identifier(struct lexer* lexer)
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
			if(lexer_current(lexer) == T_DOT)
			{
				lexer_next(lexer);

				struct node *obj = alloc_node(N_VAR);
				obj->left = symbol;

				return parse_call(lexer, obj);
			}
			else
			{
				struct node *result;
				struct node *tail;

				result = alloc_node(N_CALL);
				result->left = 0;
				result->right = parse_message(lexer, &tail, symbol);

				parse_call_tail(lexer, tail);

				return result;
			}
		}
	}
}

struct node *parse_factor(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_IF:
			return parse_if(lexer);

		case T_UNLESS:
			return parse_unless(lexer);

		case T_CASE:
			return parse_case(lexer);

		case T_NUMBER:
			{
			    struct node *result = alloc_node(N_NUMBER);

                result->left = (void *)get_token_str(lexer->token);
                result->right = (void* )atoi((char*)result->left);

			    lexer_next(lexer);

                if(lexer_current(lexer) == T_DOT)
                    return parse_call(lexer, result);

				return result;
			}

		case T_IDENT:
			return parse_identifier(lexer);

		case T_PARAM_OPEN:
			{
				lexer_next(lexer);

				struct node *result = parse_statements(lexer);

				match(lexer, T_PARAM_CLOSE);

				return result;
			}

		default:
			{
				lexer->err_count++;
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

struct node *parse_expression(struct lexer *lexer)
{
	return parse_ternary_if(lexer);
}

struct node *parse_statement(struct lexer* lexer)
{
	return parse_conditional(lexer);
}

static inline bool is_sep(struct lexer* lexer)
{
	return lexer_current(lexer) == T_SEP || lexer_current(lexer) == T_LINE;
}

static inline void skip_seps(struct lexer* lexer)
{
	while (is_sep(lexer))
		lexer_next(lexer);
}

struct node *parse_statements(struct lexer* lexer)
{
	skip_seps(lexer);

	if(is_expression(lexer))
	{
		struct node *result = parse_statement(lexer);

		if (is_sep(lexer))
		{
			skip_seps(lexer);

			if(is_expression(lexer))
			{
				struct node *node = alloc_node(N_STATEMENTS);

				node->left = result;
				node->right = parse_statements(lexer);

				return node;
			}
		}

		return result;

	}
	else
		return alloc_nil_node();
}
