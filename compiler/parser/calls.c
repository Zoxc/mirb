#include "calls.h"
#include "structures.h"
#include "../../runtime/classes/symbol.h"

node_t *parse_block(struct compiler *compiler)
{
	bool curly;

	switch(lexer_current(compiler))
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

	lexer_next(compiler);

	scope_t *scope;

	node_t *result = alloc_scope(compiler, &scope, S_CLOSURE);

	skip_lines(compiler);

	if(lexer_current(compiler) == T_OR_BINARY)
	{
		lexer_next(compiler);

		parse_parameter(compiler, scope);

		lexer_match(compiler, T_OR_BINARY);
	}

	result->right = parse_statements(compiler);

	compiler->current_scope = scope->parent;

	lexer_match(compiler, curly ? T_CURLY_CLOSE : T_END);

	return result;
}

bool has_arguments(struct compiler *compiler)
{
	if(is_expression(compiler))
	{
		switch(lexer_current(compiler))
		{
			case T_SQUARE_OPEN:
				return lexer_token(compiler)->whitespace;

			case T_ADD:
			case T_SUB:
				{
					if(lexer_token(compiler)->whitespace)
					{
						struct token token;

						lexer_context(compiler, &token);

						lexer_next(compiler);

						bool result = !lexer_token(compiler)->whitespace;

						lexer_restore(compiler, &token);

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

node_t *parse_arguments(struct compiler *compiler, bool has_args)
{
	if (has_args && lexer_current(compiler) != T_CURLY_OPEN)
	{
		if (lexer_current(compiler) == T_PARAM_OPEN && !lexer_token(compiler)->whitespace)
		{
		    node_t *result = 0;

			lexer_next(compiler);

			if(lexer_current(compiler) != T_PARAM_CLOSE)
				result = parse_argument(compiler);

			lexer_match(compiler, T_PARAM_CLOSE);

			return result;
		}
		else
			return parse_argument(compiler);
	}
	else
		return 0;
}

node_t *parse_yield(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_CALL);

	result->left = alloc_node(compiler, N_VAR);

	if(!compiler->current_scope->block_var)
		compiler->current_scope->block_var = scope_define(compiler->current_scope, 0, V_BLOCK, false);

	result->left->left = (void *)compiler->current_scope->block_var;

	result->middle = (void *)rt_symbol_from_cstr("call");

	result->right = alloc_node(compiler, N_CALL_ARGUMENTS);
	result->right->left = parse_arguments(compiler, has_arguments(compiler));
	result->right->right = 0;

	return result;
}

node_t *parse_call(struct compiler *compiler, rt_value symbol, node_t *child, bool default_var)
{
	if(!symbol)
	{
		switch(lexer_current(compiler))
		{
			case T_IDENT:
			case T_EXT_IDENT:
				{
					symbol = rt_symbol_from_lexer(compiler);

					lexer_next(compiler);
				}
				break;

			default:
				COMPILER_ERROR(compiler, "Expected identifier but found %s", token_type_names[lexer_current(compiler)]);
		}
	}

	bool local = false;

	if(symbol)
		local = scope_defined(compiler->current_scope, symbol, true);

	bool has_args = has_arguments(compiler);

	if(default_var && !has_args && (local || (symbol && rt_symbol_is_const(symbol)))) // Variable or constant
	{
		if(local)
		{
			node_t *result = alloc_node(compiler, N_VAR);

			result->left = (void *)scope_define(compiler->current_scope, symbol, V_LOCAL, true);

			return result;
		}
		else
		{
			node_t *result = alloc_node(compiler, N_CONST);

			result->left = child;
			result->right = (void *)symbol;

			return result;
		}
	}
	else // Call
	{
		node_t *result = alloc_node(compiler, N_CALL);

		result->left = child;
		result->middle = (void *)symbol;

		result->right = alloc_node(compiler, N_CALL_ARGUMENTS);
		result->right->left = parse_arguments(compiler, has_args);
		result->right->right = parse_block(compiler);

		return result;
	}
}

static inline bool is_lookup(struct compiler *compiler)
{
	return lexer_current(compiler) == T_DOT || lexer_current(compiler) == T_SQUARE_OPEN || lexer_current(compiler) == T_SCOPE;
}

node_t *parse_lookup(struct compiler *compiler, node_t *child)
{
	switch(lexer_current(compiler))
	{
		case T_SCOPE:
			{
				lexer_state(compiler, TS_NOKEYWORDS);

				lexer_next(compiler);

				lexer_state(compiler, TS_DEFAULT);

				if(lexer_require(compiler, T_IDENT))
				{
					rt_value symbol = rt_symbol_from_lexer(compiler);
					node_t *result;

					if(rt_symbol_is_const(symbol))
					{
						result = alloc_node(compiler, N_CONST);
						result->left = child;
						result->right = (void *)symbol;
					}
					else
					{
						result = alloc_node(compiler, N_CALL);
						result->left = child;
						result->middle = (void *)symbol;
						result->right = 0;
					}

					lexer_next(compiler);

					return result;
				}
				else
					return child;
			}

		case T_DOT:
			{
				lexer_state(compiler, TS_NOKEYWORDS);

				lexer_next(compiler);

				lexer_state(compiler, TS_DEFAULT);

				return parse_call(compiler, 0, child, false);
			}

		case T_SQUARE_OPEN:
			{
				lexer_next(compiler);

				node_t *result = alloc_node(compiler, N_ARRAY_CALL);

				result->left = child;
				result->middle = parse_argument(compiler);
				result->right = parse_block(compiler);

				lexer_match(compiler, T_SQUARE_CLOSE);

				return result;
			}

		default:
			RT_ASSERT(0);
	}
}

node_t *parse_lookup_tail(struct compiler *compiler, node_t *tail)
{
	if(tail && tail->type == N_CALL && tail->right)
		return tail;

	switch(lexer_current(compiler))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
			RT_ASSERT(0);

		case T_ASSIGN:
			{
				lexer_next(compiler);

				switch(tail->type)
				{
					case N_CONST:
						{
							node_t *result = alloc_node(compiler, N_ASSIGN_CONST);

							result->left = tail->left;
							result->middle = tail->right;
							result->right = parse_expression(compiler);

							free(tail);

							return result;
						}

					case N_CALL:
						{
							if(tail->middle)
							{
								const char *name = rt_symbol_to_cstr((rt_value)tail->middle);

								printf("original %s\n", name);

								char *new_name = compiler_alloc(compiler, strlen(name) + 2);
								strcpy(new_name, name);
								strcat(new_name, "=");

								printf("new %s\n", new_name);

								tail->middle = (void *)rt_symbol_from_cstr(new_name);
							}

							tail->right = alloc_node(compiler, N_CALL_ARGUMENTS);
							tail->right->right = 0;
							tail->right->left = alloc_node(compiler, N_ARGUMENT);
							tail->right->left->left = parse_expression(compiler);
							tail->right->left->right = 0;

							return tail;
						}

					case N_ARRAY_CALL:
						{
							tail->type = N_CALL;

							node_t *block = tail->right;
							node_t *argument = tail->middle;

							tail->middle = (void *)rt_symbol_from_cstr("[]=");
							tail->right = alloc_node(compiler, N_CALL_ARGUMENTS);
							tail->right->right = block;
							tail->right->left = argument;

							while(argument->right)
							{
								argument = argument->right;
							}

							argument->right = alloc_node(compiler, N_ARGUMENT);
							argument->right->left = parse_expression(compiler);
							argument->right->right = 0;

							return tail;
						}

					default:
						RT_ASSERT(0);
				}
			}
			break;

		default:
			return tail;
	}
}

node_t *parse_lookup_chain(struct compiler *compiler)
{
 	node_t *result = parse_factor(compiler);

    while(is_lookup(compiler))
    {
    	node_t *node = parse_lookup(compiler, result);

		result = node;
    }

	return parse_lookup_tail(compiler, result);
}

