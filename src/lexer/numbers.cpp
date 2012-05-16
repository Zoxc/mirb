#include "lexer.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	void Lexer::zero()
	{
		input++;

		switch(input)
		{
			case 'X':
			case 'x':
				hex();
				break;
		
			case '.':
				input++;

				if(input.in('0', '9'))
				{
					input--;
					real();
					break;
				}

				input--;
				
				lexeme.type = Lexeme::INTEGER;
				lexeme.stop = &input;
				break;
				
			case 'b':
			case 'B':
				input++;
				get_number<Lexeme::BINARY, '1'>();
				break;

			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				get_number<Lexeme::OCTAL, '7'>();
				break;

			default: // Single 0
				lexeme.type = Lexeme::INTEGER;
				lexeme.stop = &input;

				if(input == '_')
				{
					while(input == '_')
						input++;

					lexeme.stop = &input;
					parser.report(lexeme, "Trailing _ in number");
				}

				break;
		}
	}

	void Lexer::number()
	{
		input++;

		skip_numbers([&] { return input.in('0', '9'); });
		
		lexeme.stop = &input;
		lexeme.type = Lexeme::INTEGER;
		
		if(input != '.')
			return;
		
		input++;

		if(!input.in('0', '9'))
		{
			input--;
			return;
		}

		skip_numbers([&] { return input.in('0', '9'); });
		
		lexeme.stop = &input;
		lexeme.type = Lexeme::REAL;
	}

	void Lexer::real()
	{
		input++;
		
		if(input.in('0', '9'))
		{
			while(input.in('0', '9'))
				input++;

			lexeme.stop = &input;
			lexeme.type = Lexeme::REAL;
		}
		else
		{
			if(input == '.')
			{
				input++;

				if(input == '.')
				{
					input++;
					lexeme.stop = &input;
					lexeme.type = Lexeme::RANGE_EXCL;
				}
				else
				{
					lexeme.stop = &input;
					lexeme.type = Lexeme::RANGE_INCL;
				}
			}
			else
			{
				lexeme.stop = &input;
				lexeme.type = Lexeme::DOT;
			}
		}
	}

	void Lexer::hex()
	{
		input++;
		
		if(input == '_')
		{
			const char_t *start = &input;

			while(input == '_')
				input++;
			
			parser.report(range(start, &input), "Expected hex digit, but found underscore(s)");
		}

		auto is_hex = [&] { return input.in('0', '9') || input.in('A', 'F') || input.in('a', 'f'); };

		if(is_hex())
		{
			skip_numbers(is_hex);

			lexeme.stop = &input;
			lexeme.type = Lexeme::HEX;
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::HEX;

			parser.report(lexeme, "Invalid hex number");
		}
	}
};
