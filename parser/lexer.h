#pragma once
#include "../globals.h"
#include "../runtime/runtime.h"

typedef enum {
	T_NONE = 0,
	T_ADD = 1,
	T_SUB = 2,
	T_MUL = 3,
	T_DIV = 4,
	T_ASSIGN_ADD = 5,
	T_ASSIGN_SUB = 6,
	T_ASSIGN_MUL = 7,
	T_ASSIGN_DIV = 8,
	T_ASSIGN,
	T_QUESTION,
	T_DOT,
	T_COMMA,
	T_COLON,
	T_SEP,
	T_PARAM_OPEN,
	T_PARAM_CLOSE,
	T_SQUARE_OPEN,
	T_SQUARE_CLOSE,
	T_EOF,
	T_NUMBER,
	T_IDENT,
	T_LINE,

	// Keywords
	T_IF,
	T_UNLESS,
	T_ELSE,
	T_ELSIF,
	T_THEN,
	T_WHEN,
	T_CASE,
	T_CLASS,
	T_END
} token_type;

#define OP_TO_ASSIGN 4
#define T_KEYWORD_START T_IF
#define T_KEYWORD_STOP T_END

extern char *token_type_names[];

struct parser;

struct token {
    token_type type;
    char *input;
    char *start;
    char *stop;
    int line;
    struct parser *parser;
};

void parser_setup(void);
struct parser *parser_create(char *input);
void parser_destroy(struct parser *parser);

token_type next(struct parser *parser);
token_type parser_lookahead(struct parser *parser);
void parser_restore(struct parser *parser);
void parser_resolve(struct parser *parser);
char *get_token_str(struct token *token);
