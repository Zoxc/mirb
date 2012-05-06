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
				real();
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
				{
					while(input.in('0', '7'))
						input++;

					if(input.in('8', '9'))
					{
						while(input.in('0', '9'))
							input++;

						lexeme.type = Lexeme::INTEGER;
						lexeme.stop = &input;

						parser.report(lexeme.dup(memory_pool), "Invalid octal number '" + lexeme.string() + "'");
					}
					else
					{
						lexeme.type = Lexeme::OCTAL;
						lexeme.stop = &input;
					}
				}
				break;

			default: // Single 0
				lexeme.type = Lexeme::INTEGER;
				lexeme.stop = &input;
				break;
		}
	}

	void Lexer::number()
	{
		input++;

		while(input >= '0' && input <= '9')
			input++;

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
		
		while(input.in('0', '9'))
			input++;
		
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
			
			parser.report(lexeme.dup(memory_pool), "no .<digit> floating literal anymore; put 0 before dot");
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

		if(input.in('0', '9') || input.in('A', 'F'))
		{
			while(input.in('0', '9') || input.in('A', 'F'))
				input++;

			lexeme.stop = &input;
			lexeme.type = Lexeme::HEX;
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::HEX;

			parser.report(lexeme.dup(memory_pool), "Invalid hex number: '" + lexeme.string() + "'");
		}
	}
};
