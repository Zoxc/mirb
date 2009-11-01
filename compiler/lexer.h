#pragma once
#include "../globals.h"
#include "../runtime/runtime.h"

enum token_type {
	T_NONE = 0,
	T_ADD = 1,
	T_SUB = 2,
	T_MUL = 3,
	T_DIV = 4,
	T_ASSIGN_ADD = 5,
	T_ASSIGN_SUB = 6,
	T_ASSIGN_MUL = 7,
	T_ASSIGN_DIV = 8,
	T_UNARY_ADD = 9,
	T_UNARY_SUB = 10,
	T_ASSIGN,
	T_EQUALITY,
	T_CASE_EQUALITY,
	T_NO_EQUALITY,
	T_QUESTION,
	T_DOT,
	T_COMMA,
	T_COLON,
	T_SCOPE,
	T_SEP,
	T_AMP,
	T_PARAM_OPEN,
	T_PARAM_CLOSE,
	T_SQUARE_OPEN,
	T_SQUARE_CLOSE,
	T_CURLY_OPEN,
	T_CURLY_CLOSE,
	T_OR_BINARY,
	T_AND_BOOLEAN,
	T_OR_BOOLEAN,
	T_NOT_SIGN,
	T_EOF,
	T_STRING_START,
	T_STRING_CONTINUE,
	T_STRING,
	T_STRING_END,
	T_NUMBER,
	T_IVAR,
	T_IDENT,
	T_EXT_IDENT,
	T_LINE,

	// Keywords
	T_IF,
	T_UNLESS,
	T_ELSE,
	T_ELSIF,
	T_THEN,
	T_WHEN,
	T_CASE,
	T_BEGIN,
	T_ENSURE,
	T_RESCUE,
	T_CLASS,
	T_MODULE,
	T_DEF,
	T_SELF,
	T_DO,
	T_YIELD,
	T_RETURN,
	T_TRUE,
	T_FALSE,
	T_NIL,
	T_NOT,
	T_AND,
	T_OR,
	T_END
};

enum token_state {
	TS_DEFAULT,
	TS_NOKEYWORDS
};

#define OP_TO_ASSIGN (T_ASSIGN_ADD - T_ADD)
#define OP_TO_UNARY (T_UNARY_ADD - T_ADD)
#define T_KEYWORD_START T_IF
#define T_KEYWORD_STOP T_END

extern char *token_type_names[];

struct token {
    const char *input;
    const char *start;
    const char *stop;
    struct compiler *compiler;
    kvec_t(bool) curlys;
    size_t line;
	enum token_type type;
    bool whitespace;
    enum token_state state;
};

void lexer_setup(void);
void lexer_create(struct compiler *compiler, const char *input);
void lexer_destroy(struct compiler *compiler);

enum token_type lexer_next(struct compiler *compiler);

struct token *lexer_token(struct compiler *compiler);
enum token_type lexer_current(struct compiler *compiler);
void lexer_state(struct compiler *compiler, enum token_state state);
void lexer_context(struct compiler *compiler, struct token *token);
void lexer_restore(struct compiler *compiler, struct token *token);
char *get_token_str(struct token *token);

bool lexer_match(struct compiler *compiler, enum token_type type);
bool lexer_matches(struct compiler *compiler, enum token_type type);
bool lexer_require(struct compiler *compiler, enum token_type type);
