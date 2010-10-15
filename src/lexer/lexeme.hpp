#pragma once
#include "../common.hpp"
#include "../range.hpp"
#include "../symbol-pool.hpp"

namespace Mirb
{
	class Lexer;

	class Lexeme:
		public Range
	{
		public:
			enum Type
			{
				NONE,
				ADD,
				SUB,
				MUL,
				DIV,
				MOD,
				POWER,
				LEFT_SHIFT,
				RIGHT_SHIFT,
				LOGICAL_AND,
				LOGICAL_OR,
				BITWISE_XOR,
				AMPERSAND, // BITWISE_AND
				BITWISE_OR,
				ASSIGN_ADD,
				ASSIGN_SUB,
				ASSIGN_MUL,
				ASSIGN_DIV,
				ASSIGN_MOD,
				ASSIGN_POWER,
				ASSIGN_LEFT_SHIFT,
				ASSIGN_RIGHT_SHIFT,
				ASSIGN_BITWISE_XOR,
				ASSIGN_BITWISE_AND,
				ASSIGN_BITWISE_OR,
				ASSIGN_LOGICAL_AND,
				ASSIGN_LOGICAL_OR,
				BITWISE_NOT,
				LOGICAL_NOT,
				UNARY_ADD,
				UNARY_SUB,
				ASSIGN,
				EQUALITY,
				CASE_EQUALITY,
				NO_EQUALITY,
				MATCHES,
				NOT_MATCHES,
				GREATER,
				GREATER_OR_EQUAL,
				LESS,
				LESS_OR_EQUAL,
				QUESTION,
				DOT,
				COMMA,
				COLON,
				SCOPE,
				SEMICOLON,
				PARENT_OPEN,
				PARENT_CLOSE,
				SQUARE_OPEN,
				SQUARE_CLOSE,
				CURLY_OPEN,
				CURLY_CLOSE,
				STRING_START,
				STRING_CONTINUE,
				STRING,
				STRING_END,
				INTEGER,
				OCTAL,
				REAL,
				HEX,
				IVAR,
				IDENT,
				EXT_IDENT,
				LINE,
				END,
				
				// Keywords
				KW_IF,
				KW_UNLESS,
				KW_ELSE,
				KW_ELSIF,
				KW_THEN,
				KW_WHEN,
				KW_CASE,
				KW_BEGIN,
				KW_ENSURE,
				KW_RESCUE,
				KW_CLASS,
				KW_MODULE,
				KW_DEF,
				KW_SELF,
				KW_DO,
				KW_YIELD,
				KW_RETURN,
				KW_BREAK,
				KW_NEXT,
				KW_REDO,
				KW_SUPER,
				KW_TRUE,
				KW_FALSE,
				KW_NIL,
				KW_NOT,
				KW_AND,
				KW_OR,
				KW_END,
				TYPES
			};
			
			static const Type keyword_start = KW_IF;
			static const Type keyword_end = KW_END;

			Lexeme(Lexer &lexer) : lexer(lexer) {}
			
			Lexeme& operator=(const Lexeme& other);
			
			Lexer &lexer;
			
			bool whitespace : 1;
			bool allow_keywords : 1;
			
			Type type;
			const char_t *prev;
			
			//TODO: Write a vector class to avoid leaks
			std::vector<bool> curlies;
			
			union
			{
				Symbol *symbol;
				const char_t *c_str;
			};

			Range &get_prev();

			void prev_set(Range &range);
			
			static std::string names[TYPES];
			
			std::string describe();
			
			static std::string describe(Range *range, Type type);
			static std::string describe_type(Type type);
	};
};