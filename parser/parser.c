#include "../runtime/symbol.h"
#include "parser.h"
#include "calls.h"
#include "control_blocks.h"
#include "structures.h"

void parse_sep(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_LINE:
		case T_SEP:
			next(parser);
			break;

		default:
			parser->err_count++;
			printf("Excepted seperator but found %s\n", token_type_names[parser_current(parser)]);
	}
}

struct node *parse_argument(struct parser *parser)
{
	struct node *result = alloc_node(N_ARGUMENT);

	result->left = parse_expression(parser);
	result->right = 0;

	if(matches(parser, T_COMMA))
		result->right = parse_argument(parser);
	else
		result->right = 0;

	return result;
}

struct node *parse_identifier(struct parser *parser)
{
	rt_value symbol = rt_symbol_from_parser(parser);

	next(parser);

	if(is_expression(parser))
		return parse_self_call(parser, symbol);
	else
		switch (parser_current(parser))
		{
			case T_ASSIGN_ADD:
			case T_ASSIGN_SUB:
			case T_ASSIGN_MUL:
			case T_ASSIGN_DIV:
			{
				struct node *result;

				token_type op_type = parser_current(parser) - OP_TO_ASSIGN;

				next(parser);

				if (rt_symbol_is_const(symbol))
					result = alloc_node(N_ASSIGN_CONST);
				else
					result = alloc_node(N_ASSIGN);

				result->right = alloc_node(N_EXPRESSION);
				result->right->op = op_type;

				if (rt_symbol_is_const(symbol))
				{
					result->left = alloc_node(N_SELF);
					result->middle = (void *)symbol;
					result->right->left = alloc_node(N_CONST);
					result->right->left->left = alloc_node(N_SELF);
					result->right->left->right = (void *)symbol;
				}
				else
				{
					result->left = (void *)scope_define(parser->current_scope, symbol);
					result->right->left = alloc_node(N_VAR);
					result->right->left->left = (void *)scope_define(parser->current_scope, symbol);
				}

				result->right->right = parse_expression(parser);

				return result;
			}

			case T_ASSIGN:
			{
				struct node *result;

				next(parser);

				if (rt_symbol_is_const(symbol))
				{
					result = alloc_node(N_ASSIGN_CONST);
					result->left = alloc_node(N_SELF);
					result->middle = (void *)symbol;
				}
				else
				{
					result = alloc_node(N_ASSIGN);
					result->left = (void *)scope_define(parser->current_scope, symbol);
				}

				result->right = parse_expression(parser);

				return result;
			}

			// Function call or local variable

			default:
			{
				if (scope_defined(parser->current_scope, symbol))
				{
					struct node *result = alloc_node(N_VAR);

					result->left = (void *)scope_define(parser->current_scope, symbol);

					return result;
				}
				else if (rt_symbol_is_const(symbol))
				{
					struct node *result = alloc_node(N_CONST);

					result->left = alloc_node(N_SELF);
					result->right = (void *)symbol;

					return result;
				}
				else
					return parse_self_call(parser, symbol);
			}
		}
}

struct node *parse_factor(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_IF:
			return parse_if(parser);

		case T_UNLESS:
			return parse_unless(parser);

		case T_CASE:
			return parse_case(parser);

		case T_CLASS:
			return parse_class(parser);

		case T_DEF:
			return parse_method(parser);

		case T_SELF:
			{
				next(parser);

				return alloc_node(N_SELF);
			}

		case T_TRUE:
			{
				next(parser);

				return alloc_node(N_TRUE);
			}

		case T_FALSE:
			{
				next(parser);

				return alloc_node(N_FALSE);
			}

		case T_NIL:
			{
				next(parser);

				return alloc_node(N_NIL);
			}

		case T_NUMBER:
			{
			    struct node *result = alloc_node(N_NUMBER);

			    char *text = get_token_str(parser_current_token(parser));

			    result->left = (void* )atoi(text);

			    free(text);

			    next(parser);

				return result;
			}

		case T_IDENT:
			return parse_identifier(parser);

		case T_PARAM_OPEN:
			{
				next(parser);

				struct node *result = parse_statements(parser);

				match(parser, T_PARAM_CLOSE);

				return result;
			}

		default:
			{
				parser->err_count++;
				printf("Excepted expression but found %s\n", token_type_names[parser_current(parser)]);

				next(parser);

				return 0;
			}
	}
}

struct node *parse_unary(struct parser *parser)
{
    return parse_lookup_chain(parser);
}

struct node *parse_term(struct parser *parser)
{
	struct node *result = parse_unary(parser);

    while (parser_current(parser) == T_MUL || parser_current(parser) == T_DIV)
    {
    	struct node *node = alloc_node(N_TERM);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_unary(parser);
		result = node;
    }

	return result;
}

struct node *parse_arithmetic(struct parser *parser)
{
	struct node *result = parse_term(parser);

    while (parser_current(parser) == T_ADD || parser_current(parser) == T_SUB)
    {
    	struct node *node = alloc_node(N_EXPRESSION);

		node->op = parser_current(parser);
		node->left = result;

		next(parser);

		node->right = parse_term(parser);
		result = node;
    }

    return result;
}

struct node *parse_expression(struct parser *parser)
{
	return parse_ternary_if(parser);
}

struct node *parse_statement(struct parser *parser)
{
	return parse_conditional(parser);
}

static inline bool is_sep(struct parser *parser)
{
	return parser_current(parser) == T_SEP || parser_current(parser) == T_LINE;
}

static inline void skip_seps(struct parser *parser)
{
	while (is_sep(parser))
		next(parser);
}

struct node *parse_statements(struct parser *parser)
{
	skip_seps(parser);

	if(is_expression(parser))
	{
		struct node *result = parse_statement(parser);

		if (is_sep(parser))
		{
			skip_seps(parser);

			if(is_expression(parser))
			{
				struct node *node = alloc_node(N_STATEMENTS);

				node->left = result;
				node->right = parse_statements(parser);

				return node;
			}
		}

		return result;

	}
	else
		return alloc_nil_node();
}

struct node *parse_main(struct parser *parser)
{
	struct node *result = alloc_scope(parser, S_MAIN);

	result->right = parse_statements(parser);

	return result;
}

