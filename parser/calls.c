#include "calls.h"
#include "../runtime/symbol.h"

struct node *parse_block(struct parser *parser)
{
	bool curly = parser_current(parser) == T_CURLY_OPEN;
	next(parser);

	scope_t *scope;

	struct node *result = alloc_scope(parser, &scope, S_CLOSURE);

	result->right = parse_statements(parser);

	parser->current_scope = scope->owner;

	match(parser, curly ? T_CURLY_CLOSE : T_END);

	return result;
}

struct node *parse_call(struct parser *parser, rt_value symbol, struct node *child, bool default_var)
{
	if(!symbol && require(parser, T_IDENT))
	{
		symbol = rt_symbol_from_parser(parser);

		next(parser);
	}

	bool has_args = false;
	bool local = false;

	if(symbol)
		local = scope_defined(parser->current_scope, symbol, true);

	if (is_expression(parser))
	{
		switch(parser_current(parser))
		{
			case T_ADD:
			case T_SUB:
				{
					if (default_var && local)
						has_args = false;
					else if(parser_token(parser)->whitespace)
					{
						token_t token;

						parser_context(parser, &token);

						next(parser);

						has_args = !parser_token(parser)->whitespace;

						parser_restore(parser, &token);

					}
					else
						has_args = false;
				}
				break;

			default:
				has_args = true;
		}
	}

	if(default_var && !has_args && (local || (symbol && rt_symbol_is_const(symbol)))) // Variable or constant
	{
		if(local)
		{
			struct node *result = alloc_node(N_VAR);

			result->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL);

			return result;
		}
		else
		{
			struct node *result = alloc_node(N_CONST);

			result->left = child;
			result->right = (void *)symbol;

			return result;
		}
	}
	else // Call
	{
		struct node *result = alloc_node(N_CALL);

		result->left = child;
		result->middle = (void *)symbol;
		result->right = alloc_node(N_CALL_ARGUMENTS);
		result->right->left = 0;
		result->right->right = 0;

		if (has_args && parser_current(parser) != T_CURLY_OPEN)
		{
			if (parser_current(parser) == T_PARAM_OPEN && !parser_token(parser)->whitespace)
			{
				next(parser);

				if(parser_current(parser) != T_PARAM_CLOSE)
					result->right->left = parse_argument(parser);

				match(parser, T_PARAM_CLOSE);
			}
			else
				result->right->left = parse_argument(parser);
		}

		switch(parser_current(parser))
		{
		    case T_CURLY_OPEN:
		    case T_DO:
                result->right->right = parse_block(parser);
                break;

		    default:
                break;
		}

		return result;
	}
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
				parser_state(parser, TS_NOKEYWORDS);

				next(parser);

				parser_state(parser, TS_DEFAULT);

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
				parser_state(parser, TS_NOKEYWORDS);

				next(parser);

				parser_state(parser, TS_DEFAULT);

				return parse_call(parser, 0, child, false);
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
				next(parser);

				switch(tail->type)
				{
					case N_CONST:
						{
							struct node *result = alloc_node(N_ASSIGN_CONST);

							result->left = tail->left;
							result->middle = tail->right;
							result->right = parse_expression(parser);

							free(tail);

							return result;
						}

					case N_CALL:
						{
							if(tail->middle)
							{
								const char *name = rt_symbol_to_cstr((rt_value)tail->middle);

								printf("original %s\n", name);

								char *new_name = malloc(strlen(name) + 2);
								strcpy(new_name, name);
								strcat(new_name, "=");

								printf("new %s\n", new_name);

								tail->middle = (void *)rt_symbol_from_cstr(new_name);

								free(new_name);
							}

							tail->right = alloc_node(N_CALL_ARGUMENTS);
							tail->right->right = 0;
							tail->right->left = alloc_node(N_ARGUMENT);
							tail->right->left->left = parse_expression(parser);
							tail->right->left->right = 0;

							return tail;
						}

					default:
						assert(0);
				}
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

