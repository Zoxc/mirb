#include "lexer.h"
#include "parser.h"

char *token_type_names[] = {"None", "+", "-", "*", "/", "+=", "-=", "*=", "/=", "+@", "-@", "=", "==", "===", "!=", "?", ".", ",", ":", "::", ";", "&", "(", ")", "[", "]", "{", "}", "|", "&&", "||", "!", "End of File", "String{","#String", "String", "}String", "Number", "Instance variable", "Identifier", "Extended identifier", "Newline",
	"if", "unless", "else", "elsif", "then", "when", "case", "class", "module", "def", "self", "do", "yield", "true", "false", "nil", "not", "and", "or", "end"};

typedef token_type(*jump_table_entry)(token_t *token);

jump_table_entry jump_table[256];

struct parser *parser_create(const char* input)
{
	struct parser* result = malloc(sizeof(struct parser));

	result->index = 0;
	result->count = 0;
	result->err_count = 0;
    result->token.line = 0;
    result->token.type = T_NONE;
    result->token.input = input;
	result->token.parser = result;
	result->token.state = TS_DEFAULT;
	result->current_scope = 0;

    kv_init(result->token.curlys);
    allocator_init(&result->allocator);

    next(result);

    return result;
}

void parser_destroy(struct parser *parser)
{
	allocator_free(&parser->allocator);
	kv_destroy(parser->token.curlys);
	free(parser);
}

char* get_token_str(token_t *token)
{
    size_t length = (size_t)(token->stop - token->start);
    char* result = parser_alloc(token->parser, length + 1);
    memcpy(result, token->start, length);
    result[length] = 0;
    return result;
}

static inline bool compare_token(token_t *token, const char *str)
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

static inline bool is_whitespace(const char input)
{
    if(input == 0x10 || input == 0x13)
        return false;

    if(input <= 32 && input >= 1)
        return true;

    return false;
}

static inline bool skip_whitespace(const char **input)
{
	if(is_whitespace(**input))
	{
		do
		{
			(*input)++;
		}
		while(is_whitespace(**input));

		return true;
	}
	else
		return false;
}

static inline bool is_number(const char input)
{
    if(input >= '0' && input <= '9')
        return true;

    return false;
}

static token_type number_proc(token_t *token)
{
	token->start = token->input;

	(token->input)++;

	while(is_number(*(token->input)))
		(token->input)++;

	token->stop = token->input;

	return T_NUMBER;
}

static token_type unknown_proc(token_t *token)
{
    printf("Unknown character: %c\n", *(token->input));
    (token->input)++;

    return next(token->parser);
}

static token_type null_proc(token_t *token)
{
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

static token_type ivar_proc(token_t *token)
{
	if(is_ident(token->input[1]))
	{
		token->start = token->input;
		token->input++;

		while(is_ident(*(token->input)))
			token->input++;

		token->stop = token->input;

		return T_IVAR;
	}
	else
		return unknown_proc(token);
}

static token_type ident_proc(token_t *token)
{
	token->start = token->input;
	token->input++;

	while(is_ident(*(token->input)))
		token->input++;

	switch(*(token->input))
	{
		case '?':
		case '!':
			{
				token->input++;

				token->stop = token->input;

				return T_EXT_IDENT;
			}

		default:
			break;
	}

	token->stop = token->input;

	if(token->state == TS_NOKEYWORDS)
		return T_IDENT;

	//TODO: Generate a hashtable for this

	for (token_type i = T_KEYWORD_START; i <= T_KEYWORD_STOP; i++)
		if (compare_token(token, token_type_names[i]))
			return i;

	return T_IDENT;
}

static char *build_double_quote_string(const char *start, size_t length)
{
	char *result = malloc(length + 1);
	char *writer = result;
	const char *input = start;

	while(1)
		switch(*input)
		{
			case '"':
				goto done;

			case '#':
				{
					input++;

					if(*input == '{')
						goto done;
					else
					{
						*writer = '#';
						writer++;
					}
				}

			default:
				*writer = *input;
				writer++;
				input++;
		}

done:
	result[length] = 0;

	return result;
}

static token_type parse_double_quote_string(token_t *token, bool continues)
{
	const char *start = token->input;
    size_t length = 0;
    token_type result = T_STRING;

	while(1)
		switch(*(token->input))
		{
			case 0:
				return null_proc(token);

			case '\'':
				{
					token->input++;

					goto done;
				}

			case '#':
				{
					token->input++;

					if(*(token->input) == '{')
					{
						kv_push(bool, token->curlys, true);

						result = T_STRING_START;
						token->input++;

						goto done;
					}
					else
						length++;
				}

			default:
				token->input++;
				length++;
		}

done:
	if(continues)
		result += 1;

	token->start = build_double_quote_string(start, length);

	return result;
}

static token_type double_quote_proc(token_t *token)
{
	token->input++;

	return parse_double_quote_string(token, false);
}

static char *build_single_quote_string(const char *start, size_t length)
{
	char *result = malloc(length + 1);
	char *writer = result;
	const char *input = start;

	while(1)
		switch(*input)
		{
			case '\'':
				goto done;

			case '\\':
				{
					input++;

					switch(*input)
					{
						case '\\':
						case '\'':
							*writer = *input;
							writer++;
							input++;
							break;

						default:
							*writer = '\\';
							writer++;
					}
				}

			default:
				*writer = *input;
				writer++;
				input++;
		}

done:
	result[length] = 0;

	return result;
}

static token_type single_quote_proc(token_t *token)
{
	token->input++;

	const char *start = token->input;
    size_t length = 0;

	while(1)
		switch(*(token->input))
		{
			case 0:
				return null_proc(token);

			case '\'':
				{
					token->input++;

					goto done;
				}

			case '\\':
				{
					token->input++;

					switch(*(token->input))
					{
						case '\\':
						case '\'':
							length++;
							token->input++;
							break;

						default:
							length++;
					}
				}

			default:
				token->input++;
				length++;
		}

done:
	token->start = build_single_quote_string(start, length);

	return T_STRING;
}

static token_type curly_open_proc(token_t *token)
{
	token->input++;

	kv_push(bool, token->curlys, false);

	return T_CURLY_OPEN;
}

static token_type curly_close_proc(token_t *token)
{
	token->input++;

	if(kv_size(token->curlys) == 0)
		return T_CURLY_CLOSE;
	else
	{
		bool string = kv_pop(token->curlys);

		if(string)
			return parse_double_quote_string(token, true);
		else
			return T_CURLY_CLOSE;
	}
}

static token_type colon_proc(token_t *token)
{
	token->input++;

	if(*(token->input) == ':')
	{
		token->input++;

		return T_SCOPE;
	}

    return T_COLON;
}

static token_type or_sign_proc(token_t *token)
{
	token->input++;

	if(*(token->input) == '|')
	{
		token->input++;

		return T_OR_BOOLEAN;
	}

    return T_OR_BINARY;
}

static token_type amp_proc(token_t *token)
{
	token->input++;

	if(*(token->input) == '&')
	{
		token->input++;

		return T_AND_BOOLEAN;
	}

    return T_AMP;
}

static token_type assign_proc(token_t *token)
{
	token->input++;

	if(*(token->input) == '=')
	{
		token->input++;

		if(*(token->input) == '=')
		{
			token->input++;

			return T_CASE_EQUALITY;
		}

		return T_EQUALITY;
	}

    return T_ASSIGN;
}

static token_type not_sign_proc(token_t *token)
{
	token->input++;

	if(*(token->input) == '=')
	{
		token->input++;

		return T_NO_EQUALITY;
	}

    return T_NOT_SIGN;
}

#define SINGLE_PROC(name, result) static token_type name##_proc(token_t *token)\
	{\
		token->input++;\
		\
		return result;\
	}

SINGLE_PROC(param_open, T_PARAM_OPEN);
SINGLE_PROC(param_close, T_PARAM_CLOSE);
SINGLE_PROC(question, T_QUESTION);
SINGLE_PROC(sep, T_SEP);
SINGLE_PROC(dot, T_DOT);
SINGLE_PROC(comma, T_COMMA);
SINGLE_PROC(square_open, T_SQUARE_OPEN);
SINGLE_PROC(square_close, T_SQUARE_CLOSE);

#define ASSIGN_PROC(name, result) static token_type name##_proc(token_t *token)\
	{\
		token->input++;\
	\
		if(*token->input == '=')\
		{\
			token->input++;\
			\
			return result + OP_TO_ASSIGN;\
		}\
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
	jump_table['{'] = curly_open_proc;
	jump_table['}'] = curly_close_proc;
	jump_table['"'] = double_quote_proc;
	jump_table['\''] = single_quote_proc;
	jump_table['&'] = amp_proc;
	jump_table['!'] = not_sign_proc;
	jump_table['|'] = or_sign_proc;
	jump_table['@'] = ivar_proc;

    jump_table[0] = null_proc;
}

void parser_context(struct parser *parser, token_t *token)
{
	memcpy(token, &parser->token, sizeof(token_t));

	kv_dup(bool, token->curlys, parser->token.curlys);
}

void parser_restore(struct parser *parser, token_t *token)
{
	kv_destroy(parser->token.curlys);

	memcpy(&parser->token, token, sizeof(token_t));
}

void parser_state(struct parser *parser, token_state state)
{
	parser->token.state = state;
}

inline token_type next(struct parser *parser)
{
	parser->token.whitespace = skip_whitespace(&parser->token.input);

    parser->token.type = jump_table[(unsigned char)*(parser->token.input)](&parser->token);

    return parser->token.type;
}
