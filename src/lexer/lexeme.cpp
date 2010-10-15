#include "lexer.hpp"
#include "../symbol-pool.hpp"

namespace Mirb
{
	std::string Lexeme::names[TYPES] = {
		"None",
		"+",
		"-",
		"*",
		"/",
		"%",
		"**",
		"<<",
		">>",
		"&&",
		"||",
		"^",
		"&",
		"|",
		"+=",
		"-=",
		"*=",
		"/=",
		"%=",
		"**=",
		"<<=",
		">>=",
		"^=",
		"&=",
		"|=",
		"&&=",
		"||=",
		"~",
		"!",
		"+@",
		"-@",
		"=",
		"==",
		"===",
		"!=",
		"=~",
		"!~",
		">",
		">=",
		"<",
		"<=",
		"?",
		".",
		",",
		":",
		"::",
		";",
		"(",
		")",
		"[",
		"]",
		"{",
		"}",
		"opening-string",
		"continuing-string",
		"string",
		"ending-string",
		"integer",
		"octal",
		"real",
		"hex",
		"Instance variable",
		"identifier",
		"extended identifier",
		"newline",
		"end of file",
		
		// keywords
		"if",
		"unless",
		"else",
		"elsif",
		"then",
		"when",
		"case",
		"begin",
		"ensure",
		"rescue",
		"class",
		"module",
		"def",
		"self",
		"do",
		"yield",
		"return",
		"break",
		"next",
		"redo",
		"super",
		"true",
		"false",
		"nil",
		"not",
		"and",
		"or",
		"end",
	};

	Lexeme& Lexeme::operator=(const Lexeme& other)
	{
		if(this == &other)
			return *this;
		
		Range::operator=(other);
		
		whitespace = other.whitespace;
		allow_keywords = other.allow_keywords;
		type = other.type;
		prev = other.prev;
		curlies = other.curlies;
		
		return *this;
	}

	void Lexeme::prev_set(Range &range)
	{
		range.stop = prev;
	}
	
	std::string Lexeme::describe(Range *range, Type type)
	{
		std::string result;
		
		if((type >= NONE && type < keyword_start))
			result = "'" + names[type] + "'";
		else if(type >= keyword_start && type <= keyword_end)
			result = "'" + names[type] + "' (keyword)";
		else if(type == IDENT)
			result = "'" + range->string() + "' (identifier)";
		else
			result = range->string() + " (" + names[type] + ")";
		
		return result;
	}
	
	std::string Lexeme::describe()
	{
		return describe(this, this->type);
	}

	std::string Lexeme::describe_type(Type type)
	{
		std::string result;
		
		if((type >= NONE && type < keyword_start))
			result = "'" + names[type] + "'";
		else if(type >= keyword_start && type <= keyword_end)
			result = "'" + names[type] + "' (keyword)";
		else if(type == IDENT)
			result = names[type];
		
		return result;
	}
	
	Range &Lexeme::get_prev()
	{
		Range &result = *new (lexer.memory_pool) Range;
		result.start = prev;
		result.stop = prev + 1;
		result.line = line;

		const char_t *c = start;

		while(c != prev)
		{
			switch(*c)
			{
				case '\n':
					if(c - 1 != prev && c[-1] == '\r')
						c--;
				
				case '\r':
					result.line--;

				default:
					c--;
			}
		}

		const char_t *input = lexer.input_str;

		switch(*c)
		{
			case '\n':
				c--;
				if(c != input && *c == '\r')
					c--;
				break;

			case '\r':
				c--;
				break;
			
			default:
				break;
		}

		while(c != input)
		{
			switch(*c)
			{
				case '\n':
				case '\r':
					c++;
					goto end;
				
				default:
					c--;
			}
		}

		end:

		result.line_start = c;

		return result;
	}
};