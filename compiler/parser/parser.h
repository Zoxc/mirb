#pragma once
#include "../../globals.h"
#include "../../runtime/classes.h"
#include "../../runtime/runtime.h"
#include "lexer.h"
#include "../allocator.h"
#include "../ast.h"

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
	struct scope_t *parent;
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
	const char* filename;
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

node_t *parse_factor(struct parser *parser);
node_t *parse_expression(struct parser *parser);
node_t *parse_argument(struct parser *parser);
node_t *parse_arithmetic(struct parser *parser);
node_t *parse_boolean_or(struct parser *parser);
node_t *parse_low_boolean(struct parser *parser);
node_t *parse_statement(struct parser *parser);
node_t *parse_statements(struct parser *parser);
void parse_sep(struct parser *parser);
node_t *parse_main(struct parser *parser);

#define PARSER_ERROR(parser, msg, ...) do \
	{ \
		parser->err_count++; \
		printf("(%s: %d) "msg"\n", parser->filename ? parser->filename : "Line ", parser_token(parser)->line + 1, ##__VA_ARGS__); \
	} \
	while(0)

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
        PARSER_ERROR(parser, "Expected token %s but found %s", token_type_names[type], token_type_names[parser_current(parser)]);

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
        PARSER_ERROR(parser, "Expected token %s but found %s", token_type_names[type], token_type_names[parser_current(parser)]);

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

static inline void *parser_alloc(struct parser *parser, size_t length)
{
	return allocator_alloc(&parser->allocator, length);
}

static inline node_t *alloc_scope(struct parser *parser, scope_t **scope_var, scope_type type)
{
	node_t *result = parser_alloc(parser, sizeof(node_t));
	scope_t *scope = parser_alloc(parser, sizeof(scope_t));

	scope->parser = parser;
	scope->variables = kh_init(scope);

	for(int i = 0; i < VARIABLE_TYPES; i++)
		scope->var_count[i] = 0;

	scope->type = type;
	scope->parent = parser->current_scope;
	scope->owner = parser->current_scope ? parser->current_scope->owner : 0;
	scope->block_var = 0;

	result->left = (void *)scope;
	result->type = N_SCOPE;

	parser->current_scope = scope;

	*scope_var = scope;

	return result;
}

static inline node_t *alloc_node(struct parser *parser, node_type_t type)
{
	node_t *result = parser_alloc(parser, sizeof(node_t));
	result->type = type;

	return result;
}

static inline node_t *alloc_nil_node(struct parser *parser)
{
	node_t *result = parser_alloc(parser, sizeof(node_t));
	result->type = N_NIL;

	return result;
}
