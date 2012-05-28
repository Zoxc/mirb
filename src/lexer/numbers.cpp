#include "lexer.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	bool Lexer::exponent()
	{
		if(input == 'e' || input == 'E')
		{
			input++;

			if(input == '+' || input == '-')
				input++;

			std::string result;
			skip_numbers(result, [&] { return input.in('0', '9'); });
			
			lexeme.type = Lexeme::REAL;
			lexeme.stop = &input;

			return true;
		}
		else
			return false;
	}

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
				
				lexeme.number = new (memory_pool) DataEntry;
				lexeme.number->set<MemoryPool>("0", memory_pool);
				lexeme.type = Lexeme::INTEGER;
				lexeme.stop = &input;
				break;
				
			case 'b':
			case 'B':
				input++;
				get_number<Lexeme::BINARY, '1'>();
				break;
				
			case '_':
			case 'o':
			case 'O':
				input++;
				get_number<Lexeme::OCTAL, '7'>();
				break;

			case 'd':
			case 'D':
				input++;
				get_number<Lexeme::INTEGER, '9'>();
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
				lexeme.number = new (memory_pool) DataEntry;
				lexeme.number->set<MemoryPool>("0", memory_pool);
				lexeme.type = Lexeme::INTEGER;
				lexeme.stop = &input;

				if(input == '_')
				{
					while(input == '_')
						input++;

					lexeme.stop = &input;
					report(lexeme, "Trailing _ in number");
				}

				exponent();

				break;
		}
	}

	void Lexer::number()
	{
		std::string result;

		skip_numbers(result, [&] { return input.in('0', '9'); });
		
		lexeme.stop = &input;
		lexeme.type = Lexeme::INTEGER;
		lexeme.number = new (memory_pool) DataEntry;
		lexeme.number->set<MemoryPool>(result, memory_pool);

		if(exponent() || input != '.')
			return;
		
		input++;

		if(!input.in('0', '9'))
		{
			input--;
			return;
		}

		skip_numbers(result, [&] { return input.in('0', '9'); });
		
		lexeme.stop = &input;
		lexeme.type = Lexeme::REAL;

		exponent();
	}

	void Lexer::real()
	{
		input++;
		
		if(input.in('0', '9'))
		{
			std::string result;

			skip_numbers(result, [&] { return input.in('0', '9'); });

			lexeme.stop = &input;
			lexeme.type = Lexeme::REAL;

			exponent();
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
			
			report(range(start, &input), "Expected hex digit, but found underscore(s)");
		}

		auto is_hex = [&] { return input.in('0', '9') || input.in('A', 'F') || input.in('a', 'f'); };

		if(is_hex())
		{
			std::string result;

			skip_numbers(result, is_hex);
			
			lexeme.number = new (memory_pool) DataEntry;
			lexeme.number->set<MemoryPool>(result, memory_pool);

			lexeme.stop = &input;
			lexeme.type = Lexeme::HEX;
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::HEX;

			report(lexeme, "Invalid hex number");
		}
	}
};
