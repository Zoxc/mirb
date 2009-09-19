#pragma once
#include "../globals.h"
#include "../runtime/classes.h"
#include "../runtime/runtime.h"
#include "lexer.h"
#include "allocator.h"

typedef enum {
	N_NUMBER,
	N_VAR,
	N_IVAR,
	N_IVAR_ASSIGN,
	N_STRING,
	N_STRING_START,
	N_STRING_CONTINUE,
	N_ARRAY,
	N_ARRAY_ELEMENT,
	N_CONST,
	N_SELF,
	N_TRUE,
	N_FALSE,
	N_NIL,
	N_ASSIGN,
	N_ASSIGN_CONST,
	N_BOOLEAN,
	N_NOT,
	N_UNARY,
	N_TERM,
	N_EXPRESSION,
	N_IF,
	N_UNLESS,
	N_ARGUMENT,
	N_CALL_ARGUMENTS,
	N_CALL,
	N_ARRAY_CALL,
	N_STATEMENTS,
	N_CLASS,
	N_MODULE,
	N_SCOPE,
	N_METHOD,
	N_PARAMETER
} node_type;

struct node {
    struct node *left;
    struct node *middle;
    struct node *right;
    node_type type;
    token_type op;
};

typedef enum {
	V_PARAMETER,
	V_LOCAL,
	V_UPVAL,
	V_TEMP,
	V_BLOCK,
	V_ARGS
} variable_type;

#define VARIABLE_TYPES 6

typedef struct variable_t{
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

struct parser;

typedef struct scope_t {
	struct parser *parser;
	scope_type type;
	rt_value var_count[VARIABLE_TYPES];
	khash_t(scope) *variables;
	struct scope_t *owner;
	variable_t *block_var;
} scope_t;

bool scope_defined(scope_t *scope, rt_value name, bool recursive);
variable_t *scope_declare_var(scope_t *scope, rt_value name, variable_type type);
variable_t *scope_define(scope_t *scope, rt_value name, variable_type type, bool recursive);

struct parser {
	token_t token;
	int index;
	int count;
	int err_count;
	scope_t *current_scope;
	allocator_t allocator;
};

static inline token_t *parser_token(struct parser *parser)
{
	return &parser->token;
}

static inline token_type parser_current(struct parser *parser)
{
	return parser->token.type;
}

struct node *parse_factor(struct parser *parser);
struct node *parse_expression(struct parser *parser);
struct node *parse_argument(struct parser *parser);
struct node *parse_arithmetic(struct parser *parser);
struct node *parse_boolean(struct parser *parser);
struct node *parse_low_boolean(struct parser *parser);
struct node *parse_statement(struct parser *parser);
struct node *parse_statements(struct parser *parser);
void parse_sep(struct parser *parser);
struct node *parse_main(struct parser *parser);

struct node *parse_expressions(struct parser *parser);

static inline void skip_lines(struct parser *parser)
{
	while(parser_current(parser) == T_LINE)
		next(parser);
}

static inline bool match(struct parser *parser, token_type type)
{
    if (parser_current(parser) == type)
    {
        next(parser);

        return true;
    }
    else
    {
    	parser->err_count++;

        printf("Expected token %s but found %s\n", token_type_names[type], token_type_names[parser_current(parser)]);

        return false;
    }
}

static inline bool matches(struct parser *parser, token_type type)
{
    if (parser_current(parser) == type)
    {
        next(parser);

        return true;
    }
    else
        return false;
}

static inline bool require(struct parser *parser, token_type type)
{
    if (parser_current(parser) == type)
        return true;
    else
    {
    	parser->err_count++;

        printf("Expected token %s but found %s\n", token_type_names[type], token_type_names[parser_current(parser)]);

        return false;
    }
}

static inline bool is_expression(struct parser *parser)
{
	switch (parser_current(parser))
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
		case T_PARAM_OPEN:
		case T_SQUARE_OPEN:
			return true;

		default:
			return false;
	}
}

static inline void *parser_alloc(struct parser *parser, size_t length)
{
	return allocator_alloc(&parser->allocator, length);
}

static inline struct node *alloc_scope(struct parser *parser, scope_t **scope_var, scope_type type)
{
	struct node *result = parser_alloc(parser, sizeof(struct node));
	scope_t *scope = parser_alloc(parser, sizeof(scope_t));

	scope->parser = parser;
	scope->variables = kh_init(scope);

	for(int i = 0; i < VARIABLE_TYPES; i++)
		scope->var_count[i] = 0;

	scope->type = type;
	scope->owner = parser->current_scope;
	scope->block_var = 0;

	result->left = (void *)scope;
	result->type = N_SCOPE;

	parser->current_scope = scope;

	*scope_var = scope;

	return result;
}

static inline struct node *alloc_node(struct parser *parser, node_type type)
{
	struct node *result = parser_alloc(parser, sizeof(struct node));
	result->type = type;

	return result;
}

static inline struct node *alloc_nil_node(struct parser *parser)
{
	struct node *result = parser_alloc(parser, sizeof(struct node));
	result->type = N_NIL;

	return result;
}
