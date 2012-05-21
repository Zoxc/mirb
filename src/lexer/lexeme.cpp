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
		"end",
	};

	void Lexeme::prev_set(SourceLoc *range)
	{
		range->stop = prev;
	}
	
	std::string Lexeme::describe(SourceLoc *range, Type type)
	{
		std::string result;
		
		if((type >= values_end && type < keyword_start))
			result = "'" + names[type] + "'";
		else if(type >= keyword_start && type <= keyword_end)
			result = "'" + names[type] + "' (keyword)";
		else
			result = "'" + range->get_line() + "' (" + names[type] + ")";
		
		return result;
	}
	
	std::string Lexeme::describe()
	{
		return describe(this, this->type);
	}

	std::string Lexeme::describe_type(Type type)
	{
		std::string result;
		
		if((type >= values_end && type < keyword_start))
			result = "'" + names[type] + "'";
		else if(type >= keyword_start && type <= keyword_end)
			result = "'" + names[type] + "' (keyword)";
		else
			result = names[type];
		
		return result;
	}
};
