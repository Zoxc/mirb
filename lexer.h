#pragma once

typedef enum {
    T_NONE = 0,
    T_EOF,
    T_NUMBER,
    T_ADD,
    T_SUB,
    T_MUL,
    T_DIV,
    T_LINE
} token_type;

extern char* token_type_names[];

struct token {
    token_type type;
    char* input;
    char* start;
    char* stop;
    int line;
};

extern struct token current_token;

void lex(char* input);
inline token_type next(void);
void setup_lexer(void);
char* get_token_str(struct token *token);

