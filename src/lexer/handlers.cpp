#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	void Lexer::white()
	{
		input++;
		
		while(input.in(1, 9) || input.in(11, 12) || input.in(14, 32))
			input++;

		restep(true);
	}

	void Lexer::unknown(bool restep)
	{
		input++;
		
		while(jump_table[input] == &Lexer::unknown_trampoline)
			input++;

		lexeme.stop = &input;
		lexeme.type = Lexeme::NONE;

		if (lexeme.length() == 1)
			parser.report(lexeme.dup(memory_pool), "Invalid character '" + lexeme.string() + "'");
		else
			parser.report(lexeme.dup(memory_pool), "Invalid characters '" + lexeme.string() + "'");

		if(restep)
			this->restep();
	}
	
	void Lexer::unknown_trampoline()
	{
		unknown(true);
	}
	
	bool Lexer::process_null(const char_t *input, bool expected)
	{
		bool result = (size_t)(input - input_str) >= length;
		
		if(!result && !expected)
			report_null();
			
		return result;
	}

	void Lexer::null()
	{
		if(process_null(&input, true))
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::END;
		}
		else
			unknown();
	}

	void Lexer::newline()
	{
		input++;
		lexeme.line++;
		lexeme.line_start = &input;
	
		lexeme.start = &input;
		lexeme.stop = &input;
		lexeme.type = Lexeme::LINE;
	}

	void Lexer::carrige_return()
	{
		input++;
		lexeme.line++;

		if(input == '\n')
			input++;

		lexeme.line_start = &input;

		lexeme.start = &input;
		lexeme.stop = &input;
		lexeme.type = Lexeme::LINE;
	}

	void Lexer::eol()
	{
		while(input != '\n' && input != '\r' && input != 0)
			input++;
	}
	
	void Lexer::curly_open()
	{
		input++;
		
		lexeme.curlies.push(false);
		
		lexeme.stop = &input;
		lexeme.type = Lexeme::CURLY_OPEN;
	}

	void Lexer::exclamation()
	{
		input++;
		
		switch(input)
		{
			case '=':
			{
				input++;
				lexeme.type = Lexeme::NO_EQUALITY;
				
				break;
			}
			
			case '~':
			{
				input++;
				lexeme.type = Lexeme::NOT_MATCHES;
				
				break;
			}
			
			default:
			{
				lexeme.type = Lexeme::LOGICAL_NOT;
				
				break;
			}
		}
		
		lexeme.stop = &input;
	}
	
	void Lexer::colon()
	{
		input++;

		if(input == ':')
		{
			input++;

			lexeme.type = Lexeme::SCOPE;
		}
		else
			lexeme.type = Lexeme::COLON;

		lexeme.stop = &input;
	}
	
	void Lexer::assign_equal()
	{
		input++;
		
		switch(input)
		{
			case '=':
			{
				input++;

				if(input == '=')
				{
					input++;

					lexeme.type = Lexeme::CASE_EQUALITY;
				}
				else			
					lexeme.type = Lexeme::EQUALITY;
				
				break;
			}
			
			case '~':
			{
				input++;
				lexeme.type = Lexeme::MATCHES;
				
				break;
			}
			
			default:
			{
				lexeme.type = Lexeme::ASSIGN;
				
				break;
			}
		}

		lexeme.stop = &input;
	}
	
	void Lexer::compare()
	{
		input++;
		
		switch(input)
		{
			case '<':
				input++;
				if(input == '=')
				{
					input++;
					lexeme.stop = &input;
					lexeme.type = Lexeme::ASSIGN_LEFT_SHIFT;
				}
				else
				{
					lexeme.stop = &input;
					lexeme.type = Lexeme::LEFT_SHIFT;
				}
				break;
				
			case '=':
				input++;
				if(input == '>')
				{
					input++;
					lexeme.stop = &input;
					lexeme.type = Lexeme::COMPARE;
				}
				else
				{
					lexeme.stop = &input;
					lexeme.type = Lexeme::LESS_OR_EQUAL;
				}
				break;
				
			default:
				lexeme.stop = &input;
				lexeme.type = Lexeme::LESS;
		}
	}
	
	void Lexer::comment()
	{
		eol();
		restep();
	}
	
	bool Lexer::is_ident(char_t c)
	{
		return Input::char_in(c, 'a', 'z') || Input::char_in(c, 'A', 'Z') || Input::char_in(c, '0', '9') || c == '_';
	}
	
	void Lexer::skip_ident()
	{
		while(is_ident(input))
			input++;
	}
	
	void Lexer::ivar(bool restep)
	{
		const char_t *ptr = &input;
		
		if(is_ident(ptr[1]))
		{
			input++;
			
			skip_ident();
			
			lexeme.stop = &input;
			lexeme.type = Lexeme::IVAR;
			lexeme.symbol = symbol_pool.get(lexeme);
		}
		else
		{
			unknown(restep);
		}
	}
	
	void Lexer::ivar_trampoline()
	{
		ivar(true);
	}
	
	void Lexer::add()
	{
		input++;
		
		if(input == '=')
		{
			input++;
			lexeme.type = Lexeme::ASSIGN_ADD;
		}
		else if(input == '@')
		{
			input++;
			lexeme.type = Lexeme::UNARY_ADD;
		}
		else
		{
			lexeme.type = Lexeme::ADD;
		}

		lexeme.stop = &input;
	}
	
	void Lexer::sub()
	{
		input++;
		
		if(input == '=')
		{
			input++;
			lexeme.type = Lexeme::ASSIGN_SUB;
		}
		else if(input == '@')
		{
			input++;
			lexeme.type = Lexeme::UNARY_SUB;
		}
		else
		{
			lexeme.type = Lexeme::SUB;
		}

		lexeme.stop = &input;
	}

	void Lexer::ident()
	{
		input++;
		
		skip_ident();

		switch(input)
		{
			case '?':
			case '!':
			{
				input++;

				lexeme.type = Lexeme::EXT_IDENT;
				
				break;
			}

			default:
			{
				lexeme.type = Lexeme::IDENT;
				
				break;
			}
		}
		
		lexeme.stop = &input;
		lexeme.symbol = symbol_pool.get(lexeme);
		
		if(lexeme.allow_keywords)
		{
			Lexeme::Type keyword = keywords.mapping.try_get(lexeme.symbol, [] { return Lexeme::NONE; });
			
			if(keyword != Lexeme::NONE)
				lexeme.type = keyword;
		}
	}
};
