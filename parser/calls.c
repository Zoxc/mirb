#include "calls.h"
#include "structures.h"
#include "../runtime/classes/symbol.h"

struct node *parse_block(struct parser *parser)
{
	bool curly;

	switch(parser_current(parser))
	{
		case T_CURLY_OPEN:
			curly = true;
			break;

		case T_DO:
			curly = false;
			break;

		default:
			return 0;
	}

	next(parser);

	scope_t *scope;

	struct node *result = alloc_scope(parser, &scope, S_CLOSURE);

	skip_lines(parser);

	if(parser_current(parser) == T_OR_BINARY)
	{
		next(parser);

		parse_parameter(parser, scope);

		match(parser, T_OR_BINARY);
	}

	result->right = parse_statements(parser);

	parser->current_scope = scope->owner;

	match(parser, curly ? T_CURLY_CLOSE : T_END);

	return result;
}

bool has_arguments(struct parser *parser)
{
	if(is_expression(parser))
	{
		switch(parser_current(parser))
		{
			case T_SQUARE_OPEN:
				return parser_token(parser)->whitespace;

			case T_ADD:
			case T_SUB:
				{
					if(parser_token(parser)->whitespace)
					{
						token_t token;

						parser_context(parser, &token);

						next(parser);

						bool result = !parser_token(parser)->whitespace;

						parser_restore(parser, &token);

						return result;
					}
					else
						return false;
				}
				break;

			default:
				return true;
		}
	}
	else
		return false;
}

struct node *parse_arguments(struct parser *parser, bool has_args)
{
	if (has_args && parser_current(parser) != T_CURLY_OPEN)
	{
		if (parser_current(parser) == T_PARAM_OPEN && !parser_token(parser)->whitespace)
		{
		    struct node *result = 0;

			next(parser);

			if(parser_current(parser) != T_PARAM_CLOSE)
				result = parse_argument(parser);

			match(parser, T_PARAM_CLOSE);

			return result;
		}
		else
			return parse_argument(parser);
	}
	else
		return 0;
}

struct node *parse_yield(struct parser *parser)
{
	next(parser);

	struct node *result = alloc_node(parser, N_CALL);

	result->left = alloc_node(parser, N_VAR);

	if(!parser->current_scope->block_var)
		parser->current_scope->block_var = scope_define(parser->current_scope, 0, V_BLOCK, false);

	result->left->left = (void *)parser->current_scope->block_var;

	result->middle = (void *)rt_symbol_from_cstr("call");

	result->right = alloc_node(parser, N_CALL_ARGUMENTS);
	result->right->left = parse_arguments(parser, has_arguments(parser));
	result->right->right = 0;

	return result;
}

struct node *parse_call(struct parser *parser, rt_value symbol, struct node *child, bool default_var)
{
	if(!symbol)
	{
		switch(parser_current(parser))
		{
			case T_IDENT:
			case T_EXT_IDENT:
				{
					symbol = rt_symbol_from_parser(parser);

					next(parser);
				}
				break;

			default:
				{
					parser->err_count++;

					printf("Expected identifier but found %s\n", token_type_names[parser_current(parser)]);
				}
		}
	}

	bool local = false;

	if(symbol)
		local = scope_defined(parser->current_scope, symbol, true);

	bool has_args = has_arguments(parser);

	if(default_var && !has_args && (local || (symbol && rt_symbol_is_const(symbol)))) // Variable or constant
	{
		if(local)
		{
			struct node *result = alloc_node(parser, N_VAR);

			result->left = (void *)scope_define(parser->current_scope, symbol, V_LOCAL, true);

			return result;
		}
		else
		{
			struct node *result = alloc_node(parser, N_CONST);

			result->left = child;
			result->right = (void *)symbol;

			return result;
		}
	}
	else // Call
	{
		struct node *result = alloc_node(parser, N_CALL);

		result->left = child;
		result->middle = (void *)symbol;

		result->right = alloc_node(parser, N_CALL_ARGUMENTS);
		result->right->left = parse_arguments(parser, has_args);
		result->right->right = parse_block(parser);

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
						result = alloc_node(parser, N_CONST);
						result->left = child;
						result->right = (void *)symbol;
					}
					else
					{
						result = alloc_node(parser, N_CALL);
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

				struct node *result = alloc_node(parser, N_ARRAY_CALL);

				result->left = child;
				result->middle = parse_argument(parser);
				result->right = parse_block(parser);

				match(parser, T_SQUARE_CLOSE);

				return result;
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
							struct node *result = alloc_node(parser, N_ASSIGN_CONST);

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

								char *new_name = parser_alloc(parser, strlen(name) + 2);
								strcpy(new_name, name);
								strcat(new_name, "=");

								printf("new %s\n", new_name);

								tail->middle = (void *)rt_symbol_from_cstr(new_name);
							}

							tail->right = alloc_node(parser, N_CALL_ARGUMENTS);
							tail->right->right = 0;
							tail->right->left = alloc_node(parser, N_ARGUMENT);
							tail->right->left->left = parse_expression(parser);
							tail->right->left->right = 0;

							return tail;
						}

					case N_ARRAY_CALL:
						{
							tail->type = N_CALL;

							struct node *block = tail->right;
							struct node *argument = tail->middle;

							tail->middle = (void *)rt_symbol_from_cstr("[]=");
							tail->right = alloc_node(parser, N_CALL_ARGUMENTS);
							tail->right->right = block;
							tail->right->left = argument;

							while(argument->right)
							{
								argument = argument->right;
							}

							argument->right = alloc_node(parser, N_ARGUMENT);
							argument->right->left = parse_expression(parser);
							argument->right->right = 0;

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

