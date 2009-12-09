#pragma once
#include "lexer.h"

enum node_type {
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
	N_SUPER,
	N_RETURN,
	N_NEXT,
	N_REDO,
	N_BREAK,
	N_BREAK_HANDLER,
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
};

struct node {
    struct node *left;
    struct node *middle;
    struct node *right;
    enum node_type type;
    enum token_type op;
};

rt_value get_node_name(struct node *node);
