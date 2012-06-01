#include "lexer.hpp"
#include "../symbol-pool.hpp"

namespace Mirb
{
	std::string Lexeme::names[TYPES] = {
		// values
		"string",
		"array",
		"regular expression",
		"command",
		"heredoc",
		"integer",
		"octal",
		"binary",
		"real",
		"hex",
		"instance variable",
		"class variable",
		"global variable",
		"symbol",
		"identifier",
		"extended identifier",
		
		"None",
		
		"=>",
		"`",
		"->",
		"**",
		
		// order contiguous binary operators
		
		"*",
		"/",
		"%",
		
		"+",
		"-",
		
		"<<",
		">>",
		
		"&",
		
		"^",
		"|",
		
		"&&",
		
		"||",
		
		">",
		">=",
		"<",
		"<=",
		
		"<=>",
		"==",
		"===",
		"!=",
		"=~",
		"!~",
		
		
		// keep the assigns in the same order as the operators, insert dummy value if needed
		
		"**=",
		
		"*=",
		"/=",
		"%=",
		
		"+=",
		"-=",
		
		"<<=",
		">>=",
		
		"&=",
		
		"^=",
		"|=",
		
		"&&=",
		
		"||=",
		
		
		"~",
		"!",
		"+@",
		"-@",
		"=",
		"?",
		".",
		"..",
		"...",
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
		"newline",
		"end of file",
		
		// keywords
		"if",
		"unless",
		"else",
		"elsif",
		"then",
		"while",
		"until",
		"for",
		"in",
		"when",
		"case",
		"begin",
		"ensure",
		"rescue",
		"class",
		"module",
		"def",
		"alias",
		"undef",
		"defined?",
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
		"__FILE__",
		"__LINE__",
		"BEGIN",
		"END",
		"end",
	};

	void Lexeme::prev_set(SourceLoc *range)
	{
		range->stop = prev;
	}
	
	CharArray Lexeme::describe(SourceLoc *range, Type type)
	{
		CharArray result;
		
		if((type >= values_end && type < keyword_start))
			result = "'" + CharArray(names[type]) + "'";
		else if(type >= keyword_start && type <= keyword_end)
			result = "'" + CharArray(names[type]) + "' (keyword)";
		else
			result = "'" + range->limited_string() + "' (" + CharArray(names[type]) + ")";
		
		return result;
	}
	
	CharArray Lexeme::describe()
	{
		return describe(this, this->type);
	}

	CharArray Lexeme::describe_type(Type type)
	{
		CharArray result;
		
		if((type >= values_end && type < keyword_start))
			result = "'" + CharArray(names[type]) + "'";
		else if(type >= keyword_start && type <= keyword_end)
			result = "'" + CharArray(names[type]) + "' (keyword)";
		else
			result = CharArray(names[type]);
		
		return result;
	}
};
