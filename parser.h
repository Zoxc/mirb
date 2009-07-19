#pragma once
#include "lexer.h"
#include "globals.h"

typedef enum {
	N_NUMBER,
	N_VAR,
	N_ASSIGN,
	N_TERM,
	N_EXPRESSION
} node_type;

struct node {
    void *left;
    void *right;
    node_type type;
    token_type op;
};

struct node *parse_expression(struct lexer *lexer);
bool match(struct lexer* lexer, token_type type);
