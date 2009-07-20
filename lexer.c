#include "globals.h"
#include "lexer.h"

struct token current_token;

char *token_type_names[] = {"None", "+", "-", "*", "/", "+=", "-=", "*=", "/=", "=", "?", ".", ",", ":", "End of File", "Number", "Identifier", "Newline"};

typedef token_type(*jump_table_entry)(struct token *token);

jump_table_entry jump_table[256];
token_type single_table[256];

struct lexer* lexer_create(char* input)
{
	struct lexer* result = malloc(sizeof(struct lexer));

	result->index = 0;
	result->count = 0;
	result->err_count = 0;
    result->lookaheads[0].line = 0;
    result->lookaheads[0].type = T_NONE;
    result->lookaheads[0].input = input;

    for(int i = 0; i < sizeof(result->lookaheads) / sizeof(result->lookaheads[0]); i++)
		result->lookaheads[i].lexer = result;

    lexer_next(result);

    return result;
}

void lexer_destroy(struct lexer *lexer)
{
	free(lexer);
}

char* get_token_str(struct token *token)
{
    unsigned int length = (unsigned int)(token->stop - token->start);
    char* result = malloc(length + 1);
    memcpy(result, token->start, length);
    result[length] = 0;
    return result;
}

static inline bool is_whitespace(char input)
{
    if(input == 0x10 || input == 0x13)
        return false;

    if(input <= 32 && input >= 1)
        return true;

    return false;
}

static inline void skip_whitespace(char **input)
{
    while(is_whitespace(**input))
        (*input)++;
}

static inline bool is_number(char input)
{
    if(input >= '0' && input <= '9')
        return true;

    return false;
}

static token_type number_proc(struct token *token)
{
	(token->input)++;

	while(is_number(*(token->input)))
		(token->input)++;

	token->stop = token->input;

	return T_NUMBER;
}

static token_type unknown_proc(struct token *token)
{
    printf("Unknown character: %c\n", *(token->input));
    (token->input)++;

    return lexer_next(token->lexer);
}

static token_type null_proc(struct token *token)
{
    token->stop = token->input;

    return T_EOF;
}

static token_type single_proc(struct token *token)
{
	token_type result = single_table[(unsigned char)*(token->input)];
	(token->input)++;
    token->stop = token->input;

    return result;
}

static inline bool is_ident(char input)
{
    if(input >= 'a' && input <= 'z')
        return true;

    if(input >= '0' && input <= '9')
        return true;

	if(input == '_')
		return true;

    if(input >= 'A' && input <= 'Z')
        return true;

    return false;
}

static token_type ident_proc(struct token *token)
{
	(token->input)++;

	while(is_ident(*(token->input)))
		(token->input)++;

	token->stop = token->input;

	return T_IDENT;
}

#define SINGLE_PROC(name, result) static token_type name##_proc(struct token *token)\
	{\
		(token->input)++;\
		token->stop = token->input;\
		\
		return result;\
	}

SINGLE_PROC(assign, T_ASSIGN);
SINGLE_PROC(param_open, T_PARAM_OPEN);
SINGLE_PROC(param_close, T_PARAM_CLOSE);
SINGLE_PROC(question, T_QUESTION);
SINGLE_PROC(colon, T_COLON);
SINGLE_PROC(dot, T_DOT);
SINGLE_PROC(comma, T_COMMA);

#define ASSIGN_PROC(name, result) static token_type name##_proc(struct token *token)\
	{\
		(token->input)++;\
	\
		if(*token->input == '=')\
		{\
			(token->input)++;\
			\
			token->stop = token->input;\
			\
			return result + 4;\
		}\
		\
		token->stop = token->input;\
		\
		return result;\
	}

ASSIGN_PROC(add, T_ADD)
ASSIGN_PROC(sub, T_SUB)
ASSIGN_PROC(mul, T_MUL)
ASSIGN_PROC(div, T_DIV)

static inline void create_single(char input, token_type type)
{
	single_table[(unsigned char)input] = type;
	jump_table[(unsigned char)input] = single_proc;
}

void lexer_setup(void)
{
    for(int i = 0; i < 256; i++)
    {
        jump_table[i] = unknown_proc;
        single_table[i] = T_NONE;
    }

	// Numbers

    for(int i = '0'; i <= '9'; i++)
        jump_table[i] = number_proc;

	// Identifiers

    for(int i = 'a'; i < 'z'; i++)
        jump_table[i] = ident_proc;

    for(int i = 'A'; i < 'Z'; i++)
        jump_table[i] = ident_proc;

	jump_table['_'] = ident_proc;

	jump_table['+'] = add_proc;
	jump_table['-'] = sub_proc;
	jump_table['*'] = mul_proc;
	jump_table['/'] = div_proc;

	jump_table['='] = assign_proc;
	jump_table['('] = param_open_proc;
	jump_table[')'] = param_close_proc;
	jump_table['?'] = question_proc;
	jump_table['.'] = dot_proc;
	jump_table[':'] = colon_proc;
	jump_table[','] = comma_proc;

    jump_table[0] = null_proc;
}

inline token_type lexer_lookahead(struct lexer* lexer)
{
	(lexer->index)++;

	return lexer_next(lexer);
}

void lexer_restore(struct lexer* lexer)
{
	lexer->count = lexer->index;
	lexer->index = 0;
	lexer->token = &lexer->lookaheads[0];
}

void lexer_resolve(struct lexer* lexer)
{
	lexer->lookaheads[0] = lexer->lookaheads[lexer->index];
	lexer->count = 0;
	lexer->index = 0;
	lexer->token = &lexer->lookaheads[0];
}

inline token_type lexer_next(struct lexer* lexer)
{
	struct token* token = &lexer->lookaheads[lexer->index];

	if(lexer->count)
	{
		(lexer->count)--;

		if(lexer->count == 0)
			lexer->index = 0;
	}

	lexer->token = token;

    skip_whitespace(&token->input);

    token->start = token->input;
    token->type = jump_table[(unsigned char)*(token->input)](token);

    return token->type;
}
