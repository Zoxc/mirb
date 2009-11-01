#pragma once
#include "lexer.h"

typedef enum {
	N_UNARY_OP,
	N_BINARY_OP,
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
	N_NO_EQUALITY,
	N_IF,
	N_UNLESS,
	N_RETURN,
	N_HANDLER,
	N_RESCUE,
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
} node_type_t;

typedef struct node {
    struct node *left;
    struct node *middle;
    struct node *right;
    node_type_t type;
    enum token_type op;
} node_t;

rt_value get_node_name(node_t *node);
