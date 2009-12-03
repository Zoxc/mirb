#include "lexer.h"
#include "parser/parser.h"
#include "compiler.h"

char *token_type_names[] = {"None", "+", "-", "*", "/", "+=", "-=", "*=", "/=", "+@", "-@", "=", "==", "===", "!=", ">", ">=", "<", "<=", "?", ".", ",", ":", "::", ";", "&", "(", ")", "[", "]", "{", "}", "|", "&&", "||", "!", "End of File", "String{","#String", "String", "}String", "Number", "Instance variable", "Identifier", "Extended identifier", "Newline",
	"if", "unless", "else", "elsif", "then", "when", "case", "begin", "ensure", "rescue", "class", "module", "def", "self", "do", "yield", "return", "break", "next", "redo", "true", "false", "nil", "not", "and", "or", "end"};

typedef enum token_type(*jump_table_entry)(struct token *token);

jump_table_entry jump_table[256];

void lexer_create(struct compiler *compiler, const char* input)
{
	compiler->index = 0;
	compiler->count = 0;
	compiler->token.line = 0;
	compiler->token.type = T_NONE;
	compiler->token.input = input;
	compiler->token.compiler = compiler;
	compiler->token.state = TS_DEFAULT;

	kv_init(compiler->token.curlys);

	lexer_next(compiler);
}

void lexer_destroy(struct compiler *compiler)
{
	kv_destroy(compiler->token.curlys);
}

char* get_token_str(struct token *token)
{
	size_t length = (size_t)(token->stop - token->start);
	char* result = compiler_alloc(token->compiler, length + 1);
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

static inline bool is_whitespace(const char input)
{
	if(input == '\n' || input == '\r')
	{
		return false;
	}

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

static enum token_type number_proc(struct token *token)
{
	token->start = token->input;

	(token->input)++;

	while(is_number(*(token->input)))
		(token->input)++;

	token->stop = token->input;

	return T_NUMBER;
}

static enum token_type unknown_proc(struct token *token)
{
	COMPILER_ERROR(token->compiler, "Unknown character: %c", *(token->input));
	(token->input)++;

	return lexer_next(token->compiler);
}

static enum token_type null_proc(struct token *token)
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

static enum token_type ivar_proc(struct token *token)
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

static enum token_type ident_proc(struct token *token)
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

	for (enum token_type i = T_KEYWORD_START; i <= T_KEYWORD_STOP; i++)
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

			case '\\':
				{
					input++;

					switch(*input)
					{
						case 'n':
							*writer = 10;
							writer++;
							input++;
							break;

						case 'r':
							*writer = 13;
							writer++;
							input++;
							break;

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
				break;

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
				break;

			default:
				*writer = *input;
				writer++;
				input++;
		}

done:
	result[length] = 0;

	return result;
}

static enum token_type parse_double_quote_string(struct token *token, bool continues)
{
	const char *start = token->input;
	size_t length = 0;
	enum token_type result = T_STRING;

	while(1)
		switch(*(token->input))
		{
			case 0:
				return null_proc(token);

			case '"':
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
						case 'n':
						case 'r':
							length++;
							token->input++;
							break;

						default:
							length++;
					}
				}
				break;


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
				break;

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

static enum token_type double_quote_proc(struct token *token)
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
				break;

			default:
				*writer = *input;
				writer++;
				input++;
		}

done:
	result[length] = 0;

	return result;
}

static enum token_type single_quote_proc(struct token *token)
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
				break;

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
				break;

			default:
				token->input++;
				length++;
		}

done:
	token->start = build_single_quote_string(start, length);

	return T_STRING;
}

static enum token_type curly_open_proc(struct token *token)
{
	token->input++;

	kv_push(bool, token->curlys, false);

	return T_CURLY_OPEN;
}

static enum token_type curly_close_proc(struct token *token)
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

static enum token_type colon_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == ':')
	{
		token->input++;

		return T_SCOPE;
	}

	return T_COLON;
}

static enum token_type or_sign_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == '|')
	{
		token->input++;

		return T_OR_BOOLEAN;
	}

	return T_OR_BINARY;
}

static enum token_type amp_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == '&')
	{
		token->input++;

		return T_AND_BOOLEAN;
	}

	return T_AMP;
}

static enum token_type assign_proc(struct token *token)
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

static enum token_type not_sign_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == '=')
	{
		token->input++;

		return T_NO_EQUALITY;
	}

	return T_NOT_SIGN;
}

static enum token_type greater_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == '=')
	{
		token->input++;

		return T_GREATER_OR_EQUAL;
	}

	return T_GREATER;
}

static enum token_type less_proc(struct token *token)
{
	token->input++;

	if(*(token->input) == '=')
	{
		token->input++;

		return T_LESS_OR_EQUAL;
	}

	return T_LESS;
}

static enum token_type newline_proc(struct token *token)
{
	token->input++;
	token->line++;

	return T_LINE;
}

static enum token_type carrige_return_proc(struct token *token)
{
	token->input++;
	token->line++;

	if(*token->input == '\n')
		token->input++;

	return T_LINE;
}

static enum token_type comment_proc(struct token *token)
{
	token->input++;

	while(*token->input != '\n' && *token->input != '\r' && *token->input != 0)
		token->input++;

	return lexer_next(token->compiler);
}

#define SINGLE_PROC(name, result) static enum token_type name##_proc(struct token *token)\
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

#define ASSIGN_PROC(name, result) static enum token_type name##_proc(struct token *token)\
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

void lexer_setup(void)
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

	// Newlines

	jump_table['\n'] = newline_proc;
	jump_table['\r'] = carrige_return_proc;

	// Funny signs

	// ASSIGN_PROC
	jump_table['+'] = add_proc;
	jump_table['-'] = sub_proc;
	jump_table['*'] = mul_proc;
	jump_table['/'] = div_proc;

	// SINGLE_PROC
	jump_table['('] = param_open_proc;
	jump_table[')'] = param_close_proc;
	jump_table['?'] = question_proc;
	jump_table[';'] = sep_proc;
	jump_table['.'] = dot_proc;
	jump_table[','] = comma_proc;
	jump_table['['] = square_open_proc;
	jump_table[']'] = square_close_proc;

	jump_table['='] = assign_proc;
	jump_table[':'] = colon_proc;
	jump_table['{'] = curly_open_proc;
	jump_table['}'] = curly_close_proc;
	jump_table['&'] = amp_proc;
	jump_table['!'] = not_sign_proc;
	jump_table['|'] = or_sign_proc;
	jump_table['@'] = ivar_proc;
	jump_table['>'] = greater_proc;
	jump_table['<'] = less_proc;

	// Comments

	jump_table['#'] = comment_proc;

	// Strings

	jump_table['"'] = double_quote_proc;
	jump_table['\''] = single_quote_proc;

	// Null!

	jump_table[0] = null_proc;
}

struct token *lexer_token(struct compiler *compiler)
{
	return &compiler->token;
}

enum token_type lexer_current(struct compiler *compiler)
{
	return compiler->token.type;
}

void lexer_context(struct compiler *compiler, struct token *token)
{
	memcpy(token, &compiler->token, sizeof(struct token));

	kv_dup(bool, token->curlys, compiler->token.curlys);
}

void lexer_restore(struct compiler *compiler, struct token *token)
{
	kv_destroy(compiler->token.curlys);

	memcpy(&compiler->token, token, sizeof(struct token));
}

void lexer_state(struct compiler *compiler, enum token_state state)
{
	compiler->token.state = state;
}

inline enum token_type lexer_next(struct compiler *compiler)
{
	compiler->token.whitespace = skip_whitespace(&compiler->token.input);

	compiler->token.type = jump_table[(unsigned char)*(compiler->token.input)](&compiler->token);

	return compiler->token.type;
}

bool lexer_match(struct compiler *compiler, enum token_type type)
{
	if (lexer_current(compiler) == type)
	{
		lexer_next(compiler);

		return true;
	}
	else
	{
		COMPILER_ERROR(compiler, "Expected token %s but found %s", token_type_names[type], token_type_names[lexer_current(compiler)]);

		return false;
	}
}

bool lexer_matches(struct compiler *compiler, enum token_type type)
{
	if (lexer_current(compiler) == type)
	{
		lexer_next(compiler);

		return true;
	}
	else
		return false;
}

bool lexer_require(struct compiler *compiler, enum token_type type)
{
	if (lexer_current(compiler) == type)
		return true;
	else
	{
		COMPILER_ERROR(compiler, "Expected token %s but found %s", token_type_names[type], token_type_names[lexer_current(compiler)]);

		return false;
	}
}
