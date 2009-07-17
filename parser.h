#pragma once
#include "lexer.h"
#include "globals.h"

typedef enum {
	N_FACTOR,
	N_TERM,
	N_EXPRESSION
} node_type;

struct node {
    void *left;
    void *right;
    node_type type;
    token_type op;
};

struct node *parse_expression(void);
bool match(token_type type);
