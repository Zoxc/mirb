#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	Keywords::Keywords(SymbolPool &pool) : mapping(2)
	{
		for(size_t i = (size_t)Lexeme::keyword_start; i <= (size_t)Lexeme::keyword_end; i++)
			mapping.set(pool.get(Lexeme::names[i]), (Lexeme::Type)i);
	}
	
	void(Lexer::*Lexer::jump_table[sizeof(char_t) << 8])();
	Map<char_t, char_t> Lexer::delimiter_mapping(2);
	
	template<Lexeme::Type type> void Lexer::single()
	{
		input++;
		
		lexeme.stop = &input;
		lexeme.type = type;
	}

	template<Lexeme::Type type, Lexeme::Type assign_type> void Lexer::assign()
	{
		input++;
		
		if(input == '=')
		{
			input++;
			lexeme.type = assign_type;
		}
		else
		{
			lexeme.type = type;
		}

		lexeme.stop = &input;
	}

	template<Lexeme::Type type, Lexeme::Type assign_type, char_t match, Lexeme::Type match_type, Lexeme::Type match_assign> void Lexer::assign()
	{
		input++;
		
		switch(input)
		{
			case match:
				input++;
				if(input == '=')
				{
					input++;
					lexeme.type = match_assign;
				}
				else
					lexeme.type = match_type;
				break;
				
			case '=':
				input++;
				lexeme.type = assign_type;
				break;
				
			default:
				lexeme.type = type;
		}

		lexeme.stop = &input;
	}

	void Lexer::setup_jump_table()
	{
		delimiter_mapping.set('(', ')');
		delimiter_mapping.set('{', '}');
		delimiter_mapping.set('[', ']');
		delimiter_mapping.set('<', '>');

		auto set_chars = [](char_t start, char_t stop, void (Lexer::*func)()) {
			for(size_t c = start; c <= stop; ++c)
				jump_table[(size_t)c] = func;
		};

		// Unknown
		set_chars(0, 255, &Lexer::unknown);
		
		// Stop please
		jump_table[0] = &Lexer::null;
		
		// Identifiers
		set_chars('a', 'z', &Lexer::ident);
		set_chars('A', 'Z', &Lexer::ident);
		jump_table[(size_t)'_'] = &Lexer::ident;

		// String
		jump_table[(size_t)'"'] = &Lexer::string;

		// Command
		jump_table[(size_t)'`'] = &Lexer::command;

		// Numbers
		jump_table[(size_t)'0'] = &Lexer::zero;
		jump_table[(size_t)'.'] = &Lexer::real;
		set_chars('1', '9', &Lexer::number);

		// Whitespace
		set_chars(1, 9, &Lexer::white);
		set_chars(14, 32, &Lexer::white);
		jump_table[11] = &Lexer::white;
		jump_table[12] = &Lexer::white;
		
		// Newlines
		jump_table[(size_t)'\n'] = &Lexer::newline;
		jump_table[(size_t)'\r'] = &Lexer::carrige_return;
		jump_table[(size_t)'\\'] = &Lexer::skip_line;
		
		// Arithmetic
		jump_table[(size_t)'+'] = &Lexer::add;
		jump_table[(size_t)'-'] = &Lexer::sub;
		jump_table[(size_t)'*'] = &Lexer::assign<Lexeme::MUL, Lexeme::ASSIGN_MUL, '*', Lexeme::POWER, Lexeme::ASSIGN_POWER>;
		jump_table[(size_t)'/'] = &Lexer::assign<Lexeme::DIV, Lexeme::ASSIGN_DIV>;
		jump_table[(size_t)'%'] = &Lexer::assign<Lexeme::MOD, Lexeme::ASSIGN_MOD>;
		
		// Bitwise operators
		jump_table[(size_t)'^'] = &Lexer::assign<Lexeme::BITWISE_XOR, Lexeme::ASSIGN_BITWISE_XOR>;
		jump_table[(size_t)'&'] = &Lexer::assign<Lexeme::AMPERSAND, Lexeme::ASSIGN_BITWISE_AND, '&', Lexeme::LOGICAL_AND, Lexeme::ASSIGN_LOGICAL_AND>;
		jump_table[(size_t)'|'] = &Lexer::assign<Lexeme::BITWISE_OR, Lexeme::ASSIGN_BITWISE_OR, '|', Lexeme::LOGICAL_OR, Lexeme::ASSIGN_LOGICAL_OR>;
		jump_table[(size_t)'~'] = &Lexer::single<Lexeme::BITWISE_NOT>;
		
		// Logical operators
		jump_table[(size_t)'!'] = &Lexer::exclamation;
		
		// Misc
		jump_table[(size_t)'('] = &Lexer::single<Lexeme::PARENT_OPEN>;
		jump_table[(size_t)')'] = &Lexer::single<Lexeme::PARENT_CLOSE>;
		
		jump_table[(size_t)'['] = &Lexer::single<Lexeme::SQUARE_OPEN>;
		jump_table[(size_t)']'] = &Lexer::single<Lexeme::SQUARE_CLOSE>;
		
		jump_table[(size_t)';'] = &Lexer::single<Lexeme::SEMICOLON>;
		jump_table[(size_t)','] = &Lexer::single<Lexeme::COMMA>;
		jump_table[(size_t)'?'] = &Lexer::single<Lexeme::QUESTION>;

		jump_table[(size_t)'?'] = &Lexer::character;
		
		jump_table[(size_t)':'] = &Lexer::colon;
		
		jump_table[(size_t)'='] = &Lexer::assign_equal;
		
		jump_table[(size_t)'<'] = &Lexer::compare;
		jump_table[(size_t)'>'] = &Lexer::assign<Lexeme::GREATER, Lexeme::GREATER_OR_EQUAL, '>', Lexeme::RIGHT_SHIFT, Lexeme::ASSIGN_RIGHT_SHIFT>;
		
		jump_table[(size_t)'{'] = &Lexer::curly_open;

		jump_table[(size_t)'}'] = &Lexer::curly_close;

		jump_table[(size_t)'$'] = &Lexer::global;
		
		jump_table[(size_t)'@'] = &Lexer::ivar;
		jump_table[(size_t)'#'] = &Lexer::comment;
		
		jump_table[(size_t)'\''] = &Lexer::simple_string;
		jump_table[(size_t)'\"'] = &Lexer::string;
	}
	
	void Lexer::report_null()
	{
		const char_t *start = lexeme.start;
		lexeme.start = &input - 1;
		lexeme.stop = &input;
		parser.report(lexeme, "Unexpected null terminator");
		lexeme.start = start;
	}
	
	SourceLoc Lexer::range(const char_t *start, const char_t *stop)
	{
		SourceLoc result(lexeme);
		
		result.start = start;
		result.stop = stop;

		return result;
	}

	Lexer::Lexer(SymbolPool &symbol_pool, MemoryPool memory_pool, Parser &parser) : symbol_pool(symbol_pool), parser(parser), memory_pool(memory_pool), keywords(symbol_pool), lexeme(memory_pool), heredocs(memory_pool)
	{
	}
	
	void Lexer::done()
	{
		for(auto heredoc: heredocs)
			parser.report(heredoc->range, "Missing heredoc");
	}
	
	void Lexer::load(const char_t *input, size_t length)
	{
		this->input_str = input;
		this->input.set(input);
		this->length = length;
		
		lexeme.start = input;
		lexeme.current_line_start = input;
		lexeme.current_line = 0;
		lexeme.allow_keywords = true;
		
		step();
	}
	
	void Lexer::step()
	{
		lexeme.prev = lexeme.stop;
		restep();
	}

	void Lexer::restep(bool whitespace)
	{
		lexeme.start = &input;
		lexeme.error = false;
		lexeme.whitespace = whitespace;
		lexeme.line = lexeme.current_line;
		lexeme.line_start = lexeme.current_line_start;

#ifdef DEBUG
		lexeme.type = Lexeme::TYPES;
#endif

		(this->*jump_table[input])();

		mirb_debug_assert((size_t)lexeme.stop >= (size_t)lexeme.start);
		mirb_debug_assert(lexeme.type != Lexeme::TYPES);
	}
};
