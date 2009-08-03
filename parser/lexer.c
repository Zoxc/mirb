#include "lexer.h"
#include "parser.h"

char *token_type_names[] = {"None", "+", "-", "*", "/", "+=", "-=", "*=", "/=", "=", "?", ".", ",", ":", "::", ";", "(", ")", "[", "]", "End of File", "Number", "Identifier", "Newline",
	"if", "unless", "else", "elsif", "then", "when", "case", "class", "def", "end"};

typedef token_type(*jump_table_entry)(struct token *token);

jump_table_entry jump_table[256];

struct parser *parser_create(char* input)
{
	struct parser* result = malloc(sizeof(struct parser));

	result->index = 0;
	result->count = 0;
	result->err_count = 0;
    result->lookaheads[0].line = 0;
    result->lookaheads[0].type = T_NONE;
    result->lookaheads[0].input = input;

    for(int i = 0; i < sizeof(result->lookaheads) / sizeof(result->lookaheads[0]); i++)
		result->lookaheads[i].parser = result;

    next(result);

    return result;
}

void parser_destroy(struct parser *parser)
{
	free(parser);
}

char* get_token_str(struct token *token)
{
    unsigned int length = (unsigned int)(token->stop - token->start);
    char* result = malloc(length + 1);
    memcpy(result, token->start, length);
    result[length] = 0;
    return result;
}

static inline bool compare_token(struct token *token, const char *str)
{
	const char *start = token->start;
	const char *stop = token->stop;
	const char *input = str;

	while(*start == *input)
	{
		if (start >= stop)
			return false;

		start++;
		input++;

		if (*input == 0)
			return start == stop;
	}

	return false;
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

    return next(token->parser);
}

static token_type null_proc(struct token *token)
{
    token->stop = token->input;

    return T_EOF;
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
	token->input++;

	while (is_ident(*(token->input)))
		token->input++;

	token->stop = token->input;

	//TODO: Generate a hashtable for this

	for (token_type i = T_KEYWORD_START; i <= T_KEYWORD_STOP; i++)
		if (compare_token(token, token_type_names[i]))
			return i;

	return T_IDENT;
}

static token_type colon_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == ':')
	{
		token->input++;
		token->stop = token->input;

		return T_SCOPE;
	}

    token->stop = token->input;

    return T_COLON;
}

#define SINGLE_PROC(name, result) static token_type name##_proc(struct token *token)\
	{\
		token->input++;\
		token->stop = token->input;\
		\
		return result;\
	}

SINGLE_PROC(assign, T_ASSIGN);
SINGLE_PROC(param_open, T_PARAM_OPEN);
SINGLE_PROC(param_close, T_PARAM_CLOSE);
SINGLE_PROC(question, T_QUESTION);
SINGLE_PROC(sep, T_SEP);
SINGLE_PROC(dot, T_DOT);
SINGLE_PROC(comma, T_COMMA);
SINGLE_PROC(square_open, T_SQUARE_OPEN);
SINGLE_PROC(square_close, T_SQUARE_CLOSE);


#define ASSIGN_PROC(name, result) static token_type name##_proc(struct token *token)\
	{\
		token->input++;\
	\
		if(*token->input == '=')\
		{\
			token->input++;\
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

void parser_setup(void)
{
    for(int i = 0; i < 256; i++)
        jump_table[i] = unknown_proc;

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
	jump_table[';'] = sep_proc;
	jump_table[','] = comma_proc;
	jump_table['['] = square_open_proc;
	jump_table[']'] = square_close_proc;

    jump_table[0] = null_proc;
}

inline token_type parser_lookahead(struct parser *parser)
{
	parser->index++;

	return next(parser);
}

void parser_restore(struct parser *parser)
{
	parser->count = parser->index;
	parser->index = 0;
	parser->token = &parser->lookaheads[0];
}

void parser_resolve(struct parser *parser)
{
	parser->lookaheads[0] = parser->lookaheads[parser->index];
	parser->count = 0;
	parser->index = 0;
	parser->token = &parser->lookaheads[0];
}

inline token_type next(struct parser *parser)
{
	struct token* token = &parser->lookaheads[parser->index];

	if (parser->count)
	{
		parser->count--;

		if (parser->count == 0)
			parser->index = 0;
	}

	parser->token = token;

    skip_whitespace(&token->input);

    token->start = token->input;
    token->type = jump_table[(unsigned char)*(token->input)](token);

    return token->type;
}
