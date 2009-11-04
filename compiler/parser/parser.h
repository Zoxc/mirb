#pragma once
#include "../../globals.h"
#include "../../runtime/classes.h"
#include "../../runtime/runtime.h"
#include "../lexer.h"
#include "../ast.h"
#include "../block.h"

bool scope_defined(struct block *block, rt_value name, bool recursive);
variable_t *scope_declare_var(struct block *block, rt_value name, variable_type type);
variable_t *scope_define(struct block *block, rt_value name, variable_type type, bool recursive);

node_t *parse_factor(struct compiler *compiler);
node_t *parse_expression(struct compiler *compiler);
node_t *parse_argument(struct compiler *compiler);
node_t *parse_arithmetic(struct compiler *compiler);
node_t *parse_boolean_or(struct compiler *compiler);
node_t *parse_low_boolean(struct compiler *compiler);
node_t *parse_statement(struct compiler *compiler);
node_t *parse_statements(struct compiler *compiler);
void parse_sep(struct compiler *compiler);
node_t *parse_main(struct compiler *compiler);

static inline void skip_lines(struct compiler *compiler)
{
	while(lexer_current(compiler) == T_LINE)
		lexer_next(compiler);
}

static inline bool is_expression(struct compiler *compiler)
{
	switch (lexer_current(compiler))
	{
		case T_IDENT:
		case T_EXT_IDENT:
		case T_IVAR:
		case T_ADD:
		case T_SUB:
		case T_NUMBER:
		case T_IF:
		case T_UNLESS:
		case T_CASE:
		case T_BEGIN:
		case T_CLASS:
		case T_MODULE:
		case T_NOT:
		case T_NOT_SIGN:
		case T_DEF:
		case T_SELF:
		case T_TRUE:
		case T_FALSE:
		case T_NIL:
		case T_STRING:
		case T_STRING_START:
		case T_YIELD:
		case T_RETURN:
		case T_PARAM_OPEN:
		case T_SQUARE_OPEN:
			return true;

		default:
			return false;
	}
}

node_t *alloc_scope(struct compiler *compiler, struct block **block_var, enum block_type type);

static inline node_t *alloc_node(struct compiler *compiler, node_type_t type)
{
	node_t *result = compiler_alloc(compiler, sizeof(node_t));
	result->type = type;

	return result;
}

extern struct node nil_node;
