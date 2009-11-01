#pragma once
#include "../../globals.h"
#include "../../runtime/classes.h"
#include "../../runtime/runtime.h"
#include "lexer.h"
#include "../allocator.h"
#include "../ast.h"
#include "../compiler.h"

typedef enum {
	V_PARAMETER,
	V_LOCAL,
	V_UPVAL,
	V_TEMP,
	V_BLOCK,
	V_ARGS
} variable_type;

#define VARIABLE_TYPES 6

typedef struct variable_t {
	struct variable_t *real;
	rt_value index;
	rt_value name;
	variable_type type;
} variable_t;

KHASH_MAP_INIT_INT(scope, variable_t *);

typedef enum {
	S_MAIN,
	S_METHOD,
	S_CLASS,
	S_MODULE,
	S_CLOSURE
} scope_type;

struct compiler;

typedef struct scope {
	struct compiler *compiler;
	scope_type type;
	rt_value var_count[VARIABLE_TYPES];
	khash_t(scope) *variables;
	struct scope *owner;
	struct scope *parent;
	variable_t *block_var;
} scope_t;

bool scope_defined(scope_t *scope, rt_value name, bool recursive);
variable_t *scope_declare_var(scope_t *scope, rt_value name, variable_type type);
variable_t *scope_define(scope_t *scope, rt_value name, variable_type type, bool recursive);

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

static inline node_t *alloc_scope(struct compiler *compiler, scope_t **scope_var, scope_type type)
{
	node_t *result = compiler_alloc(compiler, sizeof(node_t));
	scope_t *scope = compiler_alloc(compiler, sizeof(scope_t));

	scope->compiler = compiler;
	scope->variables = kh_init(scope);

	for(int i = 0; i < VARIABLE_TYPES; i++)
		scope->var_count[i] = 0;

	scope->type = type;
	scope->parent = compiler->current_scope;
	scope->owner = compiler->current_scope ? compiler->current_scope->owner : 0;
	scope->block_var = 0;

	result->left = (void *)scope;
	result->type = N_SCOPE;

	compiler->current_scope = scope;

	*scope_var = scope;

	return result;
}

static inline node_t *alloc_node(struct compiler *compiler, node_type_t type)
{
	node_t *result = compiler_alloc(compiler, sizeof(node_t));
	result->type = type;

	return result;
}

static inline node_t *alloc_nil_node(struct compiler *compiler)
{
	node_t *result = compiler_alloc(compiler, sizeof(node_t));
	result->type = N_NIL;

	return result;
}
