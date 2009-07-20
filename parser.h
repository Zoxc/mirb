#pragma once
#include "lexer.h"
#include "globals.h"

typedef enum {
	N_NUMBER,
	N_VAR,
	N_ASSIGN,
	N_TERM,
	N_EXPRESSION,
	N_IF,
	N_ELSE,
	N_ARGUMENT,
	N_MESSAGE,
	N_CALL_TAIL,
	N_CALL,
} node_type;

struct node {
    struct node *left;
    struct node *middle;
    struct node *right;
    node_type type;
    token_type op;
};

struct node *parse_expression(struct lexer *lexer);
bool match(struct lexer* lexer, token_type type);
