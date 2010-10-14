#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../compiler.hpp"

namespace Mirb
{
	Keywords::Keywords(SymbolPool &pool)
	{
		for(size_t i = (size_t)Lexeme::keyword_start; i <= (size_t)Lexeme::keyword_end; i++)
			mapping.insert(std::pair<Symbol *, Lexeme::Type>(pool.get(Lexeme::names[i]), (Lexeme::Type)i));
	}
	
	bool Lexer::jump_table_ready = 0;
	void(Lexer::*Lexer::jump_table[sizeof(char_t) << 8])();
	
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
			lexeme.stop = &input;
			lexeme.type = assign_type;
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = type;
		}
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
					lexeme.stop = &input;
					lexeme.type = match_assign;
				}
				else
				{
					lexeme.stop = &input;
					lexeme.type = match_type;
				}
				break;
				
			case '=':
				input++;
				lexeme.stop = &input;
				lexeme.type = assign_type;
				break;
				
			default:
				lexeme.stop = &input;
				lexeme.type = type;
		}
	}

	void Lexer::setup_jump_table()
	{
		if(jump_table_ready)
			return;
		
		#define forchar(name, start, stop) for(int name = start; name <= stop; name++)

		// Unknown
		forchar(c, 0, 255)
			jump_table[c] = &Lexer::unknown;
		
		// Stop please
		jump_table[0] = &Lexer::null;
		
		// Identifiers
		forchar(c, 'a', 'z')
			jump_table[c] = &Lexer::ident;
		
		forchar(c, 'A', 'Z')
			jump_table[c] = &Lexer::ident;
		
		jump_table['_'] = &Lexer::ident;

		// String
		jump_table['"'] = &Lexer::string;


		// Numbers
		jump_table['0'] = &Lexer::zero;
		jump_table['.'] = &Lexer::real;

		forchar(c, '1', '9')
			jump_table[c] = &Lexer::number;

		// Whitespace
		forchar(c, 1, 9)
			jump_table[c] = &Lexer::white;

		forchar(c, 14, 32)
			jump_table[c] = &Lexer::white;

		jump_table[11] = &Lexer::white;
		jump_table[12] = &Lexer::white;
		
		// Newlines
		jump_table['\n'] = &Lexer::newline;
		jump_table['\r'] = &Lexer::carrige_return;
		
		// Arithmetic
		jump_table['+'] = &Lexer::assign<Lexeme::ADD, Lexeme::ASSIGN_ADD>;
		jump_table['-'] = &Lexer::assign<Lexeme::SUB, Lexeme::ASSIGN_SUB>;
		jump_table['*'] = &Lexer::assign<Lexeme::MUL, Lexeme::ASSIGN_MUL, '*', Lexeme::POWER, Lexeme::ASSIGN_POWER>;
		jump_table['/'] = &Lexer::assign<Lexeme::DIV, Lexeme::ASSIGN_DIV>;
		jump_table['%'] = &Lexer::assign<Lexeme::MOD, Lexeme::ASSIGN_MOD>;
		
		// Bitwise operators
		jump_table['^'] = &Lexer::assign<Lexeme::BITWISE_XOR, Lexeme::ASSIGN_BITWISE_XOR>;
		jump_table['&'] = &Lexer::assign<Lexeme::AMPERSAND, Lexeme::ASSIGN_BITWISE_AND, '&', Lexeme::LOGICAL_AND, Lexeme::ASSIGN_LOGICAL_AND>;
		jump_table['|'] = &Lexer::assign<Lexeme::BITWISE_OR, Lexeme::ASSIGN_BITWISE_OR, '|', Lexeme::LOGICAL_OR, Lexeme::ASSIGN_LOGICAL_OR>;
		jump_table['~'] = &Lexer::single<Lexeme::BITWISE_NOT>;
		
		// Logical operators
		jump_table['!'] = &Lexer::assign<Lexeme::LOGICAL_NOT, Lexeme::NO_EQUALITY>;
		
		// Misc
		jump_table['('] = &Lexer::single<Lexeme::PARENT_OPEN>;
		jump_table[')'] = &Lexer::single<Lexeme::PARENT_CLOSE>;
		
		jump_table['['] = &Lexer::single<Lexeme::SQUARE_OPEN>;
		jump_table[']'] = &Lexer::single<Lexeme::SQUARE_CLOSE>;
		
		jump_table[';'] = &Lexer::single<Lexeme::SEMICOLON>;
		jump_table[','] = &Lexer::single<Lexeme::COMMA>;
		jump_table['?'] = &Lexer::single<Lexeme::QUESTION>;
		
		jump_table[':'] = &Lexer::assign<Lexeme::COLON, Lexeme::SCOPE>;
		
		jump_table['='] = &Lexer::assign_equal;
		
		jump_table['<'] = &Lexer::assign<Lexeme::LESS, Lexeme::LESS_OR_EQUAL, '<', Lexeme::LEFT_SHIFT, Lexeme::ASSIGN_LEFT_SHIFT>;
		jump_table['>'] = &Lexer::assign<Lexeme::GREATER, Lexeme::GREATER_OR_EQUAL, '>', Lexeme::RIGHT_SHIFT, Lexeme::ASSIGN_RIGHT_SHIFT>;
		
		jump_table['{'] = &Lexer::curly_open;
		jump_table['}'] = &Lexer::curly_close;
		
		jump_table['@'] = &Lexer::ivar;
		jump_table['#'] = &Lexer::comment;
		
		jump_table['\''] = &Lexer::simple_string;
		jump_table['\"'] = &Lexer::string;
		
		jump_table_ready = true;
	}
	
	void Lexer::report_null()
	{
		const char_t *start = lexeme.start;
		lexeme.start = &input - 1;
		lexeme.stop = &input;
		compiler.report(lexeme.dup(memory_pool), "Unexpected null terminator");
		lexeme.start = start;
	}

	Lexer::Lexer(SymbolPool &symbol_pool, MemoryPool &memory_pool, Compiler &compiler) : symbol_pool(symbol_pool), compiler(compiler), memory_pool(memory_pool), lexeme(*this), keywords(symbol_pool)
	{
		setup_jump_table();
	}
	
	void Lexer::load(const char_t *input, size_t length)
	{
		this->input_str = input;
		this->input.set(input);
		this->length = length;
		
		lexeme.start = input;
		lexeme.line_start = input;
		lexeme.line = 0;
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
		lexeme.type = (Lexeme::Type)0xBAD;

		(this->*jump_table[input])();

		#ifdef DEBUG
			assert(lexeme.stop >= lexeme.start);
			assert(lexeme.type != (Lexeme::Type)0xBAD);
		#endif
	}
};
