#pragma once
#include <Prelude/Map.hpp>
#include "../common.hpp"
#include "../generic/range.hpp"
#include "input.hpp"
#include "lexeme.hpp"

namespace Mirb
{
	class Parser;
	class Symbol;
	class SymbolPool;

	class Keywords
	{
		public:
			Keywords(SymbolPool &pool);
			
			Map<Symbol *, Lexeme::Type> mapping;
	};
	
	class Lexer
	{
		private:
			Input input;
			
			SymbolPool &symbol_pool;
			Parser &parser;
			
			static void(Lexer::*jump_table[sizeof(char_t) << 8])();

			bool process_null(const char_t *input, bool expected = false);
			void build_string(const char_t *start, char_t *str, size_t length);
			void build_simple_string(const char_t *start, char_t *str, size_t length);

			void restep(bool whitespace = false);
			
			void report_null();
			
			void parse_string(bool initial);
			
			template<Lexeme::Type type> void single();
			template<Lexeme::Type type, Lexeme::Type assign_type> void assign();
			template<Lexeme::Type type, Lexeme::Type assign_type, char_t match, Lexeme::Type match_type, Lexeme::Type match_assign> void assign();

			void eol();
			void white();
			void unknown();
			void null();
			void newline();
			void carrige_return();
			
			void assign_equal();
			void compare();
			
			void exclamation();
			
			void comment();
			
			void curly_open();
			void curly_close();
			
			void ivar();
			static bool is_ident(char_t c);
			void skip_ident();
			
			void zero();
			void number();
			void real();
			void hex();
			
			void ident();
			void string();
			void simple_string();
		public:
			class Context
			{
				friend class Lexer;
				
				private:
					Input input;
				public:
					Context(Lexer &lexer);
					
					Lexeme lexeme;
			};
			
			static void setup_jump_table();

			Lexer(SymbolPool &symbol_pool, MemoryPool &memory_pool, Parser &parser);
			
			MemoryPool &memory_pool;
			Keywords keywords;
			
			Lexeme lexeme;

			const char_t *input_str;
			size_t length;
			
			void restore(Context &context);
			
			void load(const char_t *input, size_t length);
			void step();
			void identify_keywords();
	};
};
