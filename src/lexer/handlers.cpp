#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../compiler.hpp"

namespace Mirb
{
	void Lexer::white()
	{
		input++;
		
		while(input.in(1, 9) || input.in(11, 12) || input.in(14, 32))
			input++;

		restep(true);
	}

	void Lexer::unknown()
	{
		input++;
		
		while(jump_table[input] == &Lexer::unknown)
			input++;

		lexeme.stop = &input;
		lexeme.type = Lexeme::NONE;

		if (lexeme.length() == 1)
			compiler.report(lexeme.dup(memory_pool), "Invalid character '" + lexeme.string() + "'");
		else
			compiler.report(lexeme.dup(memory_pool), "Invalid characters '" + lexeme.string() + "'");

		restep();
	}
	
	bool Lexer::process_null(const char_t *input, bool expected)
	{
		bool result = (size_t)(input - input_str) >= length;
		
		if(result && !expected)
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
		
		lexeme.curlies.push_back(false);
		
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
	
	void Lexer::ivar()
	{
		const char_t *ptr = &input;
		
		if(is_ident(ptr[1]))
		{
			input++;
			
			skip_ident();
			
			lexeme.stop = &input;
			lexeme.type = Lexeme::IVAR;
			lexeme.value = symbol_pool.get(&lexeme);
		}
		else
			unknown();
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
		
		if(lexeme.allow_keywords)
			identify_keywords();
		
		lexeme.stop = &input;
		lexeme.value = symbol_pool.get(&lexeme);
	}
};
