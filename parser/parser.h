#pragma once
#include "../globals.h"
#include "../runtime/classes.h"
#include "../runtime/runtime.h"
#include "lexer.h"

typedef enum {
	N_NUMBER,
	N_VAR,
	N_CONST,
	N_SELF,
	N_ASSIGN,
	N_ASSIGN_CONST,
	N_TERM,
	N_EXPRESSION,
	N_IF,
	N_UNLESS,
	N_NIL,
	N_ARGUMENT,
	N_CALL,
	N_ASSIGN_MESSAGE,
	N_STATEMENTS,
	N_CLASS,
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

KHASH_MAP_INIT_INT(scope, rt_value);

typedef enum {
	S_MAIN,
	S_METHOD,
	S_CLASS,
	S_MODULE
} scope_type;

typedef struct {
	scope_type type;
	rt_value next_local;
	rt_value next_param;
	khash_t(scope) *locals;
} scope_t;

static inline bool scope_defined(scope_t *scope, rt_value symbol)
{
	return kh_get(scope, scope->locals, symbol) != kh_end(scope->locals);
}

static inline rt_value scope_define(scope_t *scope, rt_value symbol)
{
	khiter_t k = kh_get(scope, scope->locals, symbol);

	if (k != kh_end(scope->locals))
		return kh_value(scope->locals, k);

	int ret;

	k = kh_put(scope, scope->locals, symbol, &ret);

	assert(ret);

	rt_value result = scope->next_local++;

	kh_value(scope->locals, k) = result;

	return result;
}

struct parser {
	struct token *token;
	int index;
	int count;
	int err_count;
	struct token lookaheads[5];
	scope_t *current_scope;
};

static inline struct token *parser_current_token(struct parser *parser)
{
	return parser->token;
}

static inline token_type parser_current(struct parser *parser)
{
	return parser->token->type;
}

struct node *parse_factor(struct parser *parser);
struct node *parse_expression(struct parser *parser);
struct node *parse_argument(struct parser *parser);
struct node *parse_arithmetic(struct parser *parser);
struct node *parse_statement(struct parser *parser);
struct node *parse_statements(struct parser *parser);
void parse_sep(struct parser *parser);
struct node *parse_main(struct parser *parser);

struct node *parse_expressions(struct parser *parser);

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

        printf("Excepted token %s but found %s\n", token_type_names[type], token_type_names[parser_current(parser)]);

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

        printf("Excepted token %s but found %s\n", token_type_names[type], token_type_names[parser_current(parser)]);

        return false;
    }
}

static inline bool is_expression(struct parser *parser)
{
	switch (parser_current(parser))
	{
		case T_IDENT:
		case T_ADD:
		case T_SUB:
		case T_NUMBER:
		case T_IF:
		case T_UNLESS:
		case T_CASE:
		case T_CLASS:
		case T_DEF:
			return true;

		default:
			return false;
	}
}

static inline struct node *alloc_scope(struct parser *parser, scope_type type)
{
	struct node *result = malloc(sizeof(struct node));
	scope_t *scope = malloc(sizeof(scope_t));

	scope->locals = kh_init(scope);
	scope->next_local = 1;
	scope->next_param = 1;
	scope->type = type;

	result->left = (void *)scope;
	result->type = N_SCOPE;

	parser->current_scope = scope;

	return result;
}

static inline struct node *alloc_node(node_type type)
{
	struct node *result = malloc(sizeof(struct node));
	result->type = type;

	return result;
}

static inline struct node *alloc_nil_node()
{
	struct node *result = malloc(sizeof(struct node));
	result->type = N_NIL;

	return result;
}
