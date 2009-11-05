#include "control_flow.h"

node_t *parse_return(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_RETURN);

	if(compiler->current_block->type == S_CLOSURE)
		compiler->current_block->owner->require_exceptions = true; // Make sure our parent can handle the return exception.

	if(is_expression(compiler))
		result->left = parse_expression(compiler);
	else
		result->left = &nil_node;

	return result;
}

node_t *parse_next(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_NEXT);

	if(compiler->current_block->type != S_CLOSURE)
		COMPILER_ERROR(compiler, "Next outside of block.");

	if(is_expression(compiler))
		result->left = parse_expression(compiler);
	else
		result->left = &nil_node;

	return result;
}

node_t *parse_redo(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_REDO);

	if(compiler->current_block->type != S_CLOSURE)
		COMPILER_ERROR(compiler, "Redo outside of block.");

	return result;
}

node_t *parse_break(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_BREAK);

	if(compiler->current_block->type == S_CLOSURE)
	{
		compiler->current_block->can_break = true; // Flag our parent should check
	}
	else
		COMPILER_ERROR(compiler, "Break outside of block.");

	if(is_expression(compiler))
		result->left = parse_expression(compiler);
	else
		result->left = &nil_node;

	return result;
}

node_t *parse_exception_handlers(struct compiler *compiler, node_t *block)
{
	node_t *parent = alloc_node(compiler, N_HANDLER);

	parent->left = block;

	if(lexer_current(compiler) == T_RESCUE)
	{
		lexer_next(compiler);

		node_t *rescue = alloc_node(compiler, N_RESCUE);
		rescue->left = parse_statements(compiler);
		rescue->right = 0;

		parent->middle = rescue;

		while(lexer_current(compiler) == T_RESCUE)
		{
			lexer_next(compiler);

			node_t *node = alloc_node(compiler, N_RESCUE);
			node->left = parse_statements(compiler);
			node->right = 0;

			rescue->right = node;
			rescue = node;
		}
	}
	else
		parent->middle = 0;

	if(lexer_current(compiler) == T_ENSURE)
	{
		lexer_next(compiler);

		parent->right = parse_statements(compiler);
	}
	else
		parent->right = 0;

	return parent;
}

node_t *parse_begin(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = parse_statements(compiler);

	switch (lexer_current(compiler))
	{
		case T_ENSURE:
		case T_RESCUE:
			result = parse_exception_handlers(compiler, result);
			break;

		default:
			break;
	}

	lexer_match(compiler, T_END);

	return result;
}

void parse_then_sep(struct compiler *compiler)
{
	switch (lexer_current(compiler))
	{
		case T_THEN:
		case T_COLON:
			lexer_next(compiler);
			break;

		default:
			parse_sep(compiler);
	}
}

node_t *parse_ternary_if(struct compiler *compiler)
{
	node_t *result = parse_boolean_or(compiler);

	if(lexer_current(compiler) == T_QUESTION)
	{
		lexer_next(compiler);

    	node_t *node = alloc_node(compiler, N_IF);

		node->left = result;
		node->middle = parse_ternary_if(compiler);

		lexer_match(compiler, T_COLON);

		node->right = parse_ternary_if(compiler);

		return node;
	}

	return result;
}

node_t *parse_conditional(struct compiler *compiler)
{
	node_t *result = parse_low_boolean(compiler);

    if (lexer_current(compiler) == T_IF || lexer_current(compiler) == T_UNLESS)
    {
     	node_t *node = alloc_node(compiler, lexer_current(compiler) == T_IF ? N_IF : N_UNLESS);

		lexer_next(compiler);

		node->middle = result;
		node->left = parse_statement(compiler);
		node->right = &nil_node;

		return node;
    }

    return result;
}

node_t *parse_unless(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_UNLESS);
	result->left = parse_expression(compiler);

	parse_then_sep(compiler);

	result->middle = parse_statements(compiler);
	result->right = &nil_node;

	lexer_match(compiler, T_END);

	return result;
}

node_t *parse_if_tail(struct compiler *compiler)
{
	switch (lexer_current(compiler))
	{
		case T_ELSIF:
			{
				lexer_next(compiler);

				node_t *result = alloc_node(compiler, N_IF);
				result->left = parse_expression(compiler);

				parse_then_sep(compiler);

				result->middle = parse_statements(compiler);
				result->right = parse_if_tail(compiler);

				return result;
			}

		case T_ELSE:
			lexer_next(compiler);

			return parse_statements(compiler);

		default:
			return &nil_node;
	}
}

node_t *parse_if(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = alloc_node(compiler, N_IF);
	result->left = parse_expression(compiler);

	parse_then_sep(compiler);

	result->middle = parse_statements(compiler);
	result->right = parse_if_tail(compiler);

	lexer_match(compiler, T_END);

	return result;
}

node_t *parse_case_body(struct compiler *compiler)
{
	switch (lexer_current(compiler))
	{
		case T_WHEN:
			{
				lexer_next(compiler);

				node_t *result = alloc_node(compiler, N_IF);
				result->left = parse_expression(compiler);

				parse_then_sep(compiler);

				result->middle = parse_statements(compiler);
				result->right = parse_case_body(compiler);

				return result;
			}

		case T_ELSE:
			{
				lexer_next(compiler);

				return parse_statements(compiler);
			}

		default:
			{
				COMPILER_ERROR(compiler, "Expected else or when but found %s", token_type_names[lexer_current(compiler)]);

				return 0;
			}
	}
}

node_t *parse_case(struct compiler *compiler)
{
	lexer_next(compiler);

	node_t *result = 0;

	switch (lexer_current(compiler))
	{
		case T_ELSE:
			lexer_next(compiler);

			result = parse_statements(compiler);
			break;

		case T_WHEN:
			result = parse_case_body(compiler);
			break;

		default:
			COMPILER_ERROR(compiler, "Expected else or when but found %s", token_type_names[lexer_current(compiler)]);
	}

	lexer_match(compiler, T_END);

	return result;
}
