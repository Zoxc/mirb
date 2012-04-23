#pragma once
#include <Prelude/Vector.hpp>
#include "../common.hpp"
#include "../symbol-pool.hpp"
#include "../generic/range.hpp"

namespace Mirb
{
	class Lexer;

	class Lexeme:
		public Range
	{
		public:
			enum Type
			{
				// values
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
				
				NONE,
				
				POWER,
				
				// order contiguous binary operators
				
				MUL,
				DIV,
				MOD,
				
				ADD,
				SUB,
				
				LEFT_SHIFT,
				RIGHT_SHIFT,
				
				AMPERSAND, // BITWISE_AND
				
				BITWISE_XOR,
				BITWISE_OR, // block parameter start/end
				
				LOGICAL_AND,
				
				LOGICAL_OR,
				
				GREATER,
				GREATER_OR_EQUAL,
				LESS,
				LESS_OR_EQUAL,
				
				COMPARE,
				EQUALITY,
				CASE_EQUALITY,
				NO_EQUALITY,
				MATCHES,
				NOT_MATCHES,
				
								
				// keep the assigns in the same order as the operators
				
				ASSIGN_POWER,
				
				ASSIGN_MUL,
				ASSIGN_DIV,
				ASSIGN_MOD,
				
				ASSIGN_ADD,
				ASSIGN_SUB,
				
				ASSIGN_LEFT_SHIFT,
				ASSIGN_RIGHT_SHIFT,
				
				ASSIGN_BITWISE_AND,
				
				ASSIGN_BITWISE_XOR,
				ASSIGN_BITWISE_OR,
				
				ASSIGN_LOGICAL_AND,
				
				ASSIGN_LOGICAL_OR,
				
				
				BITWISE_NOT,
				LOGICAL_NOT,
				
				// must be in the same order as ADD and SUB
				UNARY_ADD,
				UNARY_SUB,
				
				ASSIGN,
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
			
			static inline Type operator_to_assign(Type op)
			{
				return (Type)((size_t)op + ((size_t)ASSIGN_POWER - (size_t)POWER));
			}
			
			static inline Type assign_to_operator(Type op)
			{
				return (Type)((size_t)op - ((size_t)ASSIGN_POWER - (size_t)POWER));
			}
			
			static inline Type operator_to_unary(Type op)
			{
				return (Type)((size_t)op + ((size_t)UNARY_ADD - (size_t)ADD));
			}
			
			static const Type precedence_operators_start = MUL;
			static const Type precedence_operators_end = NOT_MATCHES;
			static const Type keyword_start = KW_IF;
			static const Type keyword_end = KW_END;
			static const Type values_end = NONE;

			Lexeme(Lexer &lexer, MemoryPool &memory_pool) : lexer(lexer), curlies(memory_pool) {}
			
			Lexeme(const Lexeme &lexeme);
			
			Lexeme& operator=(const Lexeme& other);
			
			Lexer &lexer;
			
			bool whitespace : 1;
			bool allow_keywords : 1;
			bool error : 1;
			
			Type type;
			const char_t *prev;
			
			Vector<bool, MemoryPool> curlies;
			
			union
			{
				Symbol *symbol;
				const char_t *c_str;
			};

			Range &get_prev();

			void prev_set(Range *range);
			
			static std::string names[TYPES];
			
			std::string describe();
			
			static std::string describe(Range *range, Type type);
			static std::string describe_type(Type type);
	};
};
