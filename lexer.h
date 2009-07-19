#pragma once

typedef enum {
    T_NONE = 0,
    T_EOF,
    T_NUMBER,
    T_IDENT,
    T_ADD,
    T_SUB,
    T_MUL,
    T_DIV,
    T_ASSIGN,
    T_PARAM_OPEN,
    T_PARAM_CLOSE,
    T_LINE
} token_type;

extern char *token_type_names[];

struct lexer;

struct token {
    token_type type;
    char *input;
    char *start;
    char *stop;
    int line;
    struct lexer *lexer;
};

struct lexer {
	struct token *token;
	int index;
	int count;
	int err_count;
	struct token lookaheads[5];
};

void lexer_setup(void);
struct lexer *lexer_create(char *input);
void lexer_destroy(struct lexer *lexer);
token_type lexer_next(struct lexer *lexer);
token_type lexer_lookahead(struct lexer *lexer);
void lexer_restore(struct lexer *lexer);
void lexer_resolve(struct lexer *lexer);
char *get_token_str(struct token *token);

static inline struct token* lexer_current_token(struct lexer *lexer)
{
	return lexer->token;
}

static inline token_type lexer_current(struct lexer *lexer)
{
	return lexer->token->type;
}

