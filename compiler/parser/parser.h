#pragma once
#include "../../globals.h"
#include "../../runtime/classes.h"
#include "../../runtime/runtime.h"
#include "../lexer.h"
#include "../ast.h"
#include "../block.h"

struct block *scope_defined(struct block *block, rt_value name, bool recursive);
struct variable *scope_declare_var(struct block *block, rt_value name);
struct variable *scope_var(struct block *block);
struct variable *scope_define(struct block *block, rt_value name);
struct variable *scope_get(struct block *block, rt_value name);

struct node *parse_factor(struct compiler *compiler);
struct node *parse_expression(struct compiler *compiler);
struct node *parse_argument(struct compiler *compiler);
struct node *parse_arithmetic(struct compiler *compiler);
struct node *parse_boolean_or(struct compiler *compiler);
struct node *parse_low_boolean(struct compiler *compiler);
struct node *parse_statement(struct compiler *compiler);
struct node *parse_statements(struct compiler *compiler);
void parse_sep(struct compiler *compiler);
struct node *parse_main(struct compiler *compiler);

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
		case T_BREAK:
		case T_SUPER:
		case T_REDO:
		case T_NEXT:
		case T_PARAM_OPEN:
		case T_SQUARE_OPEN:
			return true;

		default:
			return false;
	}
}

#define T_ASSIGN_OPS \
		case T_ASSIGN_ADD: \
		case T_ASSIGN_SUB: \
		case T_ASSIGN_MUL: \
		case T_ASSIGN_DIV: \
		case T_ASSIGN_LEFT_SHIFT: \
		case T_ASSIGN_RIGHT_SHIFT:

struct node *alloc_scope(struct compiler *compiler, struct block **block_var, enum block_type type);

static inline struct node *alloc_node(struct compiler *compiler, enum node_type type)
{
	struct node *result = (struct node *)compiler_alloc(compiler, sizeof(struct node));
	result->type = type;
	result->unbalanced = false;

	return result;
}

/*
 * These nodes uses no arguments and can be statically allocated.
 */
extern struct node nil_node;
extern struct node self_node;
