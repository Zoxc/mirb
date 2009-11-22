#include "../../runtime/classes/symbol.h"
#include "parser.h"
#include "calls.h"
#include "control_flow.h"
#include "structures.h"

/*
 * These nodes uses no arguments and can be statically allocated.
 */
struct node nil_node = {0, 0, 0, N_NIL, 0};
struct node self_node = {0, 0, 0, N_SELF, 0};

struct node *alloc_scope(struct compiler *compiler, struct block **block_var, enum block_type type)
{
	struct block *block = block_create(compiler, type);

	struct node *result = compiler_alloc(compiler, sizeof(struct node));

	block->parent = compiler->current_block;
	block->owner = compiler->current_block ? compiler->current_block->owner : 0;

	result->left = (void *)block;
	result->type = N_SCOPE;

	compiler->current_block = block;

	*block_var = block;

	return result;
}

struct block *scope_defined(struct block *block, rt_value name, bool recursive)
{
	if(kh_get(block, block->variables, name) != kh_end(block->variables))
		return block;

	if(recursive)
	{
		while (block->type == S_CLOSURE)
		{
			block = block->parent;

			if(kh_get(block, block->variables, name) != kh_end(block->variables))
				return block;
		}
	}

	return 0;
}

struct variable *scope_declare_var(struct block *block, rt_value name)
{
	khiter_t k = kh_get(block, block->variables, name);

	if (k != kh_end(block->variables))
		return kh_value(block->variables, k);

	int ret;

	k = kh_put(block, block->variables, name, &ret);

	RT_ASSERT(ret);

	struct variable *var = compiler_alloc(block->compiler, sizeof(struct variable));
	var->owner = block;
	var->type = V_LOCAL;
	var->name = name;
	var->real = 0;

	kh_value(block->variables, k) = var;

	return var;
}

struct variable *scope_define(struct block *block, rt_value name)
{
	if(scope_defined(block, name, false))
		return kh_value(block->variables, kh_get(block, block->variables, name));
	else
		return scope_declare_var(block, name);
}

struct variable *scope_get(struct block *block, rt_value name)
{
	struct block *defined_block = scope_defined(block, name, true);

	if(defined_block == block)
	{
		return kh_value(block->variables, kh_get(block, block->variables, name));
	}
	else if(defined_block)
	{
		struct variable *var;

		khiter_t k = kh_get(block, defined_block->variables, name);

		var = kh_value(defined_block->variables, k);
		var->type = V_HEAP;
		var->owner->heap_vars = true;

		/*
		 * Make sure the defined block is required by the current block and parents.
		 */
		while(block != defined_block)
		{
			block_require_scope(block, defined_block);

			block = block->parent;
		}

		return var;
	}
	else
		return scope_declare_var(block, name);
}

void parse_sep(struct compiler *compiler)
{
	switch (lexer_current(compiler))
	{
		case T_LINE:
		case T_SEP:
			lexer_next(compiler);
			break;

		default:
			COMPILER_ERROR(compiler, "Expected seperator but found %s", token_type_names[lexer_current(compiler)]);
	}
}

struct node *parse_argument(struct compiler *compiler)
{
	struct node *result = alloc_node(compiler, N_ARGUMENT);

	result->left = parse_expression(compiler);
	result->right = 0;

	if(lexer_matches(compiler, T_COMMA))
		result->right = parse_argument(compiler);
	else
		result->right = 0;

	return result;
}

struct node *parse_identifier(struct compiler *compiler)
{
	rt_value symbol = rt_symbol_from_lexer(compiler);

	lexer_next(compiler);

	switch (lexer_current(compiler))
	{
		case T_ASSIGN_ADD:
		case T_ASSIGN_SUB:
		case T_ASSIGN_MUL:
		case T_ASSIGN_DIV:
		{
			struct node *result;

			enum token_type op_type = lexer_current(compiler) - OP_TO_ASSIGN;

			lexer_next(compiler);

			if (rt_symbol_is_const(symbol))
				result = alloc_node(compiler, N_ASSIGN_CONST);
			else
				result = alloc_node(compiler, N_ASSIGN);

			result->right = alloc_node(compiler, N_BINARY_OP);
			result->right->op = op_type;

			if (rt_symbol_is_const(symbol))
			{
				result->left = &self_node;
				result->middle = (void *)symbol;
				result->right->left = alloc_node(compiler, N_CONST);
				result->right->left->left = &self_node;
				result->right->left->right = (void *)symbol;
			}
			else
			{
				result->left = (void *)scope_get(compiler->current_block, symbol);
				result->right->left = alloc_node(compiler, N_VAR);
				result->right->left->left = (void *)scope_get(compiler->current_block, symbol);
			}

			result->right->right = parse_expression(compiler);

			return result;
		}

		case T_ASSIGN:
		{
			struct node *result;

			lexer_next(compiler);

			if (rt_symbol_is_const(symbol))
			{
				result = alloc_node(compiler, N_ASSIGN_CONST);
				result->left = &self_node;
				result->middle = (void *)symbol;
			}
			else
			{
				result = alloc_node(compiler, N_ASSIGN);
				result->left = (void *)scope_get(compiler->current_block, symbol);
			}

			result->right = parse_expression(compiler);

			return result;
		}

		// Function call or local variable

		default:
			return parse_call(compiler, symbol, &self_node, true);
	}
}

struct node *parse_array_element(struct compiler *compiler)
{
	struct node *result = alloc_node(compiler, N_ARRAY_ELEMENT);

	result->left = parse_expression(compiler);

	if(lexer_current(compiler) == T_COMMA)
	{
		lexer_next(compiler);

		result->right  = parse_array_element(compiler);
	}
	else
		result->right = 0;

	return result;
}

struct node *parse_factor(struct compiler *compiler)
{
	switch (lexer_current(compiler))
	{
		case T_BEGIN:
			return parse_begin(compiler);

		case T_IF:
			return parse_if(compiler);

		case T_UNLESS:
			return parse_unless(compiler);

		case T_CASE:
			return parse_case(compiler);

		case T_CLASS:
			return parse_class(compiler);

		case T_MODULE:
			return parse_module(compiler);

		case T_DEF:
			return parse_method(compiler);

		case T_YIELD:
			return parse_yield(compiler);

		case T_RETURN:
			return parse_return(compiler);

		case T_BREAK:
			return parse_break(compiler);

		case T_NEXT:
			return parse_next(compiler);

		case T_REDO:
			return parse_redo(compiler);

		case T_SQUARE_OPEN:
			{
			    struct node *result = alloc_node(compiler, N_ARRAY);

			    lexer_next(compiler);

			    if(lexer_current(compiler) == T_SQUARE_CLOSE)
					result->left  = 0;
				else
					result->left = parse_array_element(compiler);

				lexer_match(compiler, T_SQUARE_CLOSE);

				return result;
			}

		case T_STRING:
			{
			    struct node *result = alloc_node(compiler, N_STRING);

				result->left = (void *)lexer_token(compiler)->start;

				lexer_next(compiler);

				return result;
			}

		case T_STRING_START:
			{
			    struct node *result = alloc_node(compiler, N_STRING_CONTINUE);

				result->left = 0;
				result->middle = (void *)lexer_token(compiler)->start;

				lexer_next(compiler);

				result->right = parse_statements(compiler);

				while(lexer_current(compiler) == T_STRING_CONTINUE)
				{
					struct node *node = alloc_node(compiler, N_STRING_CONTINUE);

					node->left = result;
					node->middle = (void *)lexer_token(compiler)->start;

					lexer_next(compiler);

					node->right = parse_statements(compiler);

					result = node;
				}

				if(lexer_require(compiler, T_STRING_END))
				{
					struct node *node = alloc_node(compiler, N_STRING_START);

					node->left = result;
					node->right = (void *)lexer_token(compiler)->start;

					lexer_next(compiler);

					return node;
				}

				return result;
			}

		case T_SELF:
			{
				lexer_next(compiler);

				return &self_node;
			}

		case T_TRUE:
			{
				lexer_next(compiler);

				return alloc_node(compiler, N_TRUE);
			}

		case T_FALSE:
			{
				lexer_next(compiler);

				return alloc_node(compiler, N_FALSE);
			}

		case T_NIL:
			{
				lexer_next(compiler);

				return &nil_node;
			}

		case T_NUMBER:
			{
			    struct node *result = alloc_node(compiler, N_NUMBER);

			    char *text = get_token_str(lexer_token(compiler));

			    result->left = (void* )atoi(text);

			    lexer_next(compiler);

				return result;
			}

		case T_IVAR:
			{
				rt_value symbol = rt_symbol_from_lexer(compiler);

				lexer_next(compiler);

				switch (lexer_current(compiler))
				{
					case T_ASSIGN_ADD:
					case T_ASSIGN_SUB:
					case T_ASSIGN_MUL:
					case T_ASSIGN_DIV:
					{
						struct node *result;

						enum token_type op_type = lexer_current(compiler) - OP_TO_ASSIGN;

						lexer_next(compiler);

						result = alloc_node(compiler, N_IVAR_ASSIGN);

						result->right = alloc_node(compiler, N_BINARY_OP);
						result->right->op = op_type;
						result->right->left = alloc_node(compiler, N_IVAR);
						result->right->left->left = (void *)symbol;
						result->right->right = parse_expression(compiler);

						result->left = (void *)symbol;

						return result;
					}

					case T_ASSIGN:
					{
						struct node *result;

						lexer_next(compiler);

						result = alloc_node(compiler, N_IVAR_ASSIGN);
						result->left = (void *)symbol;
						result->right = parse_expression(compiler);

						return result;
					}

					default:
					{
						struct node *result = alloc_node(compiler, N_IVAR);

						result->left = (void *)symbol;

						return result;
					}
				}
			}

		case T_IDENT:
			return parse_identifier(compiler);

		case T_EXT_IDENT:
			return parse_call(compiler, 0, &self_node, false);

		case T_PARAM_OPEN:
			{
				lexer_next(compiler);

				struct node *result = parse_statements(compiler);

				lexer_match(compiler, T_PARAM_CLOSE);

				return result;
			}

		default:
			{
				COMPILER_ERROR(compiler, "Expected expression but found %s", token_type_names[lexer_current(compiler)]);

				lexer_next(compiler);

				return 0;
			}
	}
}

struct node *parse_unary(struct compiler *compiler)
{
    if(lexer_current(compiler) == T_ADD || lexer_current(compiler) == T_SUB)
    {
    	struct node *result = alloc_node(compiler, N_UNARY_OP);

		result->op = lexer_current(compiler) + OP_TO_UNARY;

		lexer_next(compiler);

		result->left = parse_lookup_chain(compiler);;

		return result;
    }
    else
		return parse_lookup_chain(compiler);
}

struct node *parse_term(struct compiler *compiler)
{
	struct node *result = parse_unary(compiler);

    while(lexer_current(compiler) == T_MUL || lexer_current(compiler) == T_DIV)
    {
    	struct node *node = alloc_node(compiler, N_BINARY_OP);

		node->op = lexer_current(compiler);
		node->left = result;

		lexer_next(compiler);

		node->right = parse_unary(compiler);
		result = node;
    }

	return result;
}

struct node *parse_arithmetic(struct compiler *compiler)
{
	struct node *result = parse_term(compiler);

    while (lexer_current(compiler) == T_ADD || lexer_current(compiler) == T_SUB)
    {
    	struct node *node = alloc_node(compiler, N_BINARY_OP);

		node->op = lexer_current(compiler);
		node->left = result;

		lexer_next(compiler);

		node->right = parse_term(compiler);
		result = node;
    }

    return result;
}

struct node *parse_boolean_unary(struct compiler *compiler)
{
    if(lexer_current(compiler) == T_NOT_SIGN)
    {
    	struct node *result = alloc_node(compiler, N_NOT);

		lexer_next(compiler);

		result->left = parse_arithmetic(compiler);

		return result;
    }
    else
		return parse_arithmetic(compiler);
}

static inline bool is_equality_op(struct compiler *compiler)
{
	switch(lexer_current(compiler))
	{
		case T_EQUALITY:
		case T_CASE_EQUALITY:
		case T_NO_EQUALITY:
			return true;

		default:
			return false;
	}
}

struct node *parse_equality(struct compiler *compiler)
{
	struct node *result = parse_boolean_unary(compiler);

    while(is_equality_op(compiler))
    {
    	struct node *node;

    	if(lexer_current(compiler) == T_NO_EQUALITY)
    		node = alloc_node(compiler,  N_NO_EQUALITY);
		else
		{
			node = alloc_node(compiler,  N_BINARY_OP);
			node->op = lexer_current(compiler);
		}

		node->left = result;

		lexer_next(compiler);

		node->right = parse_boolean_unary(compiler);
		result = node;
    }

    return result;
}

struct node *parse_boolean_and(struct compiler *compiler)
{
	struct node *result = parse_equality(compiler);

    while(lexer_current(compiler) == T_AND_BOOLEAN)
    {
    	struct node *node = alloc_node(compiler, N_BOOLEAN);

		node->op = lexer_current(compiler);
		node->left = result;

		lexer_next(compiler);

		node->right = parse_equality(compiler);
		result = node;
    }

    return result;
}

struct node *parse_boolean_or(struct compiler *compiler)
{
	struct node *result = parse_boolean_and(compiler);

    while(lexer_current(compiler) == T_OR_BOOLEAN)
    {
    	struct node *node = alloc_node(compiler, N_BOOLEAN);

		node->op = lexer_current(compiler);
		node->left = result;

		lexer_next(compiler);

		node->right = parse_boolean_and(compiler);
		result = node;
    }

    return result;
}

struct node *parse_expression(struct compiler *compiler)
{
	return parse_ternary_if(compiler);
}

struct node *parse_low_boolean_unary(struct compiler *compiler)
{
    if(lexer_current(compiler) == T_NOT)
    {
    	struct node *result = alloc_node(compiler, N_NOT);

		lexer_next(compiler);

		result->left = parse_expression(compiler);

		return result;
    }
    else
		return parse_expression(compiler);
}

struct node *parse_low_boolean(struct compiler *compiler)
{
	struct node *result = parse_low_boolean_unary(compiler);

    while(lexer_current(compiler) == T_AND || lexer_current(compiler) == T_OR)
    {
    	struct node *node = alloc_node(compiler, N_BOOLEAN);

		node->op = lexer_current(compiler);
		node->left = result;

		lexer_next(compiler);

		node->right = parse_low_boolean_unary(compiler);
		result = node;
    }

    return result;
}

struct node *parse_statement(struct compiler *compiler)
{
	return parse_conditional(compiler);
}

static inline bool is_sep(struct compiler *compiler)
{
	return lexer_current(compiler) == T_SEP || lexer_current(compiler) == T_LINE;
}

static inline void skip_seps(struct compiler *compiler)
{
	while (is_sep(compiler))
		lexer_next(compiler);
}

struct node *parse_statements(struct compiler *compiler)
{
	skip_seps(compiler);

	if(is_expression(compiler))
	{
		struct node *result = parse_statement(compiler);

		if (is_sep(compiler))
		{
			skip_seps(compiler);

			if(is_expression(compiler))
			{
				struct node *node = alloc_node(compiler, N_STATEMENTS);

				node->left = result;
				node->right = parse_statements(compiler);

				return node;
			}
		}

		return result;

	}
	else
		return &nil_node;
}

struct node *parse_main(struct compiler *compiler)
{
	struct block *block;

	struct node *result = alloc_scope(compiler, &block, S_MAIN);

	block->owner = block;

	result->right = parse_statements(compiler);

	lexer_match(compiler, T_EOF);

	return result;
}

