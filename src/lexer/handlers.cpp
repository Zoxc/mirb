#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	bool Lexer::is_white()
	{
		return (input.in(1, 9) || input.in(11, 12) || input.in(14, 32));
	}
	
	void Lexer::white()
	{
		input++;
		
		while(is_white())
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
			parser.report(lexeme.dup(memory_pool), "Invalid character '" + lexeme.string() + "'");
		else
			parser.report(lexeme.dup(memory_pool), "Invalid characters '" + lexeme.string() + "'");

		restep();
	}
	
	void Lexer::skip_line()
	{
		input++;

		if(input == '\n')
		{
			newline();
			return restep();
		}
		else if(input == '\r')
		{
			carrige_return();
			return restep();
		}
		
		input--;
		unknown();
	}

	void Lexer::parse_delimited_data(Lexeme::Type type)
	{
		InterpolateState state;
		state.type = type;

		state.terminator = delimiter_mapping.try_get(input, [&]{
			return input;
		});

		input++;

		parse_interpolate(&state, false);
	}
	
	void Lexer::mod_to_literal()
	{
		switch(input)
		{
			case 'r':
				input++;
				return parse_delimited_data(Lexeme::REGEXP);

			case 's':
				input++;
				return parse_delimited_data(Lexeme::SYMBOL);

			case 'Q':
				input++;
				return parse_delimited_data(Lexeme::STRING);

			default:
				break;
		}

		if(is_alpha(input))
			return;

		return parse_delimited_data(Lexeme::STRING);
	}

	void Lexer::div_to_regexp()
	{
		input.set(lexeme.start + 1);

		InterpolateState state;
		state.type = Lexeme::REGEXP;
		state.terminator = '/';
		parse_interpolate(&state, false);
	}
	
	void Lexer::character()
	{
		input++;

		if(is_white() || input == '\0' || input == ':')
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::QUESTION;

			return;
		}
		else
		{
			char_t first = input++;
			char_t *str = new (memory_pool) char_t[1];

			if(is_start_ident(first) && is_ident(input))
			{
				input--;
				lexeme.stop = &input;
				lexeme.type = Lexeme::QUESTION;

				return;
			}
			else if(first == '\\')
			{
				std::string result;

				if(parse_escape(result))
				{
					lexeme.stop = &input;
					lexeme.data = new (memory_pool) InterpolateData(memory_pool);
					lexeme.type = Lexeme::STRING;
					parser.report(lexeme.dup(memory_pool), "Expected escape string");

					return;
				}

				str[0] = result[0];
			}
			else
				str[0] = first;

			lexeme.data = new (memory_pool) InterpolateData(memory_pool);
			lexeme.data->tail.data = str;
			lexeme.data->tail.length = 1;
			lexeme.stop = &input;
			lexeme.type = Lexeme::STRING;
		}
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
		
		lexeme.curlies.push(nullptr);
		
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
			
			case '>':
			{
				input++;
				lexeme.type = Lexeme::ASSOC;
				
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
	
	bool Lexer::is_alpha(char_t c)
	{
		return Input::char_in(c, 'a', 'z') || Input::char_in(c, 'A', 'Z') || Input::char_in(c, '0', '9');
	}
	
	bool Lexer::is_start_ident(char_t c)
	{
		return Input::char_in(c, 'a', 'z') || Input::char_in(c, 'A', 'Z') || c == '_';
	}
	
	bool Lexer::is_ident(char_t c)
	{
		return is_start_ident(c)  || Input::char_in(c, '0', '9');
	}
	
	void Lexer::skip_ident()
	{
		while(is_ident(input))
			input++;
	}
	
	void Lexer::ivar()
	{
		input++;
		
		if(is_ident(input))
		{
			bool error = !is_start_ident(input);
			
			input++;

			skip_ident();
			
			lexeme.stop = &input;
			lexeme.type = Lexeme::IVAR;
			lexeme.symbol = symbol_pool.get(lexeme);

			if(error)
				parser.report(lexeme.dup(memory_pool), "Instance variable names cannot start with a number");
		}
		else if(input == '@')
		{
			input++;

			if(is_ident(input))
			{
				bool error = !is_start_ident(input);
			
				input++;

				skip_ident();
			
				lexeme.stop = &input;
				lexeme.type = Lexeme::CVAR;
				lexeme.symbol = symbol_pool.get(lexeme);

				if(error)
					parser.report(lexeme.dup(memory_pool), "Class variable names cannot start with a number");
			}
			else
			{
				lexeme.stop = &input;
				lexeme.type = Lexeme::CVAR;
				lexeme.symbol = symbol_pool.get(lexeme);

				parser.report(lexeme.dup(memory_pool), "Expected a class variable name");
			}
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::IVAR;
			lexeme.symbol = symbol_pool.get(lexeme);

			parser.report(lexeme.dup(memory_pool), "Expected a instance variable name");
		}
	}
	
	void Lexer::global()
	{
		input++;

		if(!is_ident(input) || (input == '0' && !is_ident((&input)[1])))
		{
			switch(input)
			{
				case '0':
				case ':':
				case '*':
					input++;
					lexeme.stop = &input;
					lexeme.type = Lexeme::GLOBAL;
					lexeme.symbol = symbol_pool.get(lexeme);
					break;

				default:
					lexeme.stop = &input;
					lexeme.type = Lexeme::GLOBAL;
					lexeme.symbol = symbol_pool.get(lexeme);

					parser.report(lexeme.dup(memory_pool), "Expected a global variable name");
					break;
			}

			return;
		}

		bool error = !is_start_ident(input);
		
		skip_ident();
			
		lexeme.stop = &input;
		lexeme.type = Lexeme::GLOBAL;
		lexeme.symbol = symbol_pool.get(lexeme);

		if(error)
			parser.report(lexeme.dup(memory_pool), "Global variable names cannot start with a number");
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
