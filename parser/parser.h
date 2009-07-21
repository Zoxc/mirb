#pragma once
#include "../globals.h"
#include "lexer.h"

typedef enum {
	N_NUMBER,
	N_VAR,
	N_ASSIGN,
	N_TERM,
	N_EXPRESSION,
	N_IF,
	N_UNLESS,
	N_NIL,
	N_ARGUMENT,
	N_MESSAGE,
	N_ARRAY_MESSAGE,
	N_CALL_TAIL,
	N_CALL,
	N_EXPRESSIONS
} node_type;

struct node {
    struct node *left;
    struct node *middle;
    struct node *right;
    node_type type;
    token_type op;
};

struct node *parse_expression(struct lexer *lexer);
struct node *parse_argument(struct lexer* lexer);
struct node *parse_arithmetic(struct lexer* lexer);
struct node *parse_expressions(struct lexer* lexer);
void parse_sep(struct lexer* lexer);

struct node *parse_expressions(struct lexer* lexer);

static inline bool match(struct lexer* lexer, token_type type)
{
    if (lexer_current(lexer) == type)
    {
        lexer_next(lexer);

        return true;
    }
    else
    {
    	lexer->err_count++;

        printf("Excepted token %s but found %s\n", token_type_names[type], token_type_names[lexer_current(lexer)]);

        return false;
    }
}

static inline bool is_expression(struct lexer* lexer)
{
	switch (lexer_current(lexer))
	{
		case T_IDENT:
		case T_ADD:
		case T_SUB:
		case T_NUMBER:
		case T_IF:
		case T_UNLESS:
			return true;

		default:
			return false;
	}
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
