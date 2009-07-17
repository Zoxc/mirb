#include "globals.h"
#include "lexer.h"

struct token current_token;

char* token_type_names[] = {"None", "End of File", "Number", "+", "-", "*", "/", "Newline"};

typedef token_type(*jump_table_entry)(void);

jump_table_entry jump_table[256];

void lex(char* input)
{
    current_token.line = 0;
    current_token.type = T_NONE;
    current_token.input = input;
}

char* get_token_str(struct token *token)
{
    unsigned int length = (unsigned int)(token->stop - token->start);
    char* result = malloc(length + 1);
    memcpy(result, token->start, length);
    result[length] = 0;
    return result;
}

inline bool is_whitespace(char input)
{
    if(input == 0x10 || input == 0x13)
        return false;

    if(input <= 32 && input >= 1)
        return true;

    return false;
}

inline void skip_whitespace(char **input)
{
    while(is_whitespace(**input))
        (*input)++;
}

inline bool is_number(char input)
{
    if(input >= '0' && input <= '9')
        return true;

    return false;
}

token_type number_proc(void)
{
    current_token.input++;

    while(is_number(*current_token.input))
        current_token.input++;

    current_token.stop = current_token.input;

    return T_NUMBER;
}

token_type unknown_proc(void)
{
    printf("Unknown character: %c\n", *current_token.input);
    current_token.input++;

    return next();
}

token_type null_proc(void)
{
    current_token.stop = current_token.input;

    return T_EOF;
}

token_type add_proc(void)
{
    current_token.input++;
    current_token.stop = current_token.input;

    return T_ADD;
}

token_type sub_proc(void)
{
    current_token.input++;
    current_token.stop = current_token.input;

    return T_SUB;
}

token_type mul_proc(void)
{
    current_token.input++;
    current_token.stop = current_token.input;

    return T_MUL;
}

token_type div_proc(void)
{
    current_token.input++;
    current_token.stop = current_token.input;

    return T_DIV;
}

void setup_lexer(void)
{
    for(int i = 0; i < 256; i++)
        jump_table[i] = unknown_proc;

    for(int i = '0'; i < '9'; i++)
        jump_table[i] = number_proc;

    jump_table['+'] = add_proc;
    jump_table['-'] = sub_proc;
    jump_table['*'] = mul_proc;
    jump_table['/'] = div_proc;

    jump_table[0] = null_proc;
}

inline token_type next(void)
{
    skip_whitespace(&current_token.input);

    current_token.start = current_token.input;

    current_token.type = jump_table[(unsigned char)*current_token.input]();

    return current_token.type;
}
