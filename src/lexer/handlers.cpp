#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"
#include "../tree/nodes.hpp"

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
			parser.report(lexeme, "Invalid character '" + lexeme.string() + "'");
		else
			parser.report(lexeme, "Invalid characters '" + lexeme.string() + "'");

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

	void Lexer::left_shift_to_heredoc()
	{
		auto heredoc = new (memory_pool) Heredoc(parser.fragment);
		
		heredoc->scope = parser.scope;
		heredoc->trapper = parser.trapper;

		if(input == '-')
		{
			input++;
			heredoc->remove_ident = true;
		}
		else
			heredoc->remove_ident = false;
		
		std::string name;

		if(is_ident(input))
		{
			heredoc->type = 0;

			while(is_ident(input))
			{
				name += input;
				input++;
			}
		}
		else
		{
			auto get_name = [&]() {
				heredoc->type = input++;

				if(input == heredoc->type)
				{
					input++;

					const char_t *start = lexeme.start;

					lexeme.start = &input - 2;
					lexeme.stop = &input;
					parser.report(lexeme, "Expected heredoc delimiter");

					lexeme.start = start;
					return;
				}

				while(input != heredoc->type)
				{
					if(input == 0 && process_null(&input))
						return;

					if(input == '\n' || input == '\r')
					{
						lexeme.stop = &input;
						parser.report(lexeme, "Unterminated heredoc delimiter");
						return;
					}

					name += input;
				}

				input++;
			};

			switch(input)
			{
				case '"':
				case '\'':
					get_name();
					break;

				default:
					lexeme.stop = &input;
					parser.report(lexeme, "Expected heredoc delimiter");
					return;
			}
		}

		heredoc->name.set<MemoryPool>(name, memory_pool);

		heredocs.push(heredoc);
		
		heredoc->node = new (heredoc->fragment) Tree::HeredocNode;

		lexeme.stop = &input;

		heredoc->range = lexeme;

		lexeme.type = Lexeme::HEREDOC;
		lexeme.heredoc = heredoc->node;
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

			case 'x':
				input++;
				return parse_delimited_data(Lexeme::COMMAND);

			case 'Q':
				input++;
				return parse_delimited_data(Lexeme::STRING);
				
			case 'w':
			{
				input++;
				
				char_t terminator = delimiter_mapping.try_get(input, [&]{
					return input;
				});

				if(input == 0 && process_null(&input))
					return;

				input++;

				const char_t *start = &input;

				while(input != terminator)
				{
					if(input == 0 && process_null(&input))
						return;

					input++;
				}
				
				std::string result((const char *)start, (size_t)&input - (size_t)start);

				lexeme.stop = &input;

				lexeme.data = new (memory_pool) InterpolateData(memory_pool);
				lexeme.data->tail.set<MemoryPool>(result, memory_pool);
				lexeme.type = Lexeme::ARRAY;
				
				input++;

				return;
			}

			case 'W':
				input++;
				return parse_delimited_data(Lexeme::ARRAY);

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
					parser.report(lexeme, "Expected escape string");

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
	
	void Lexer::parse_interpolate_heredoc(Heredoc *heredoc)
	{
		InterpolateState state;
		state.type = Lexeme::STRING;
		state.heredoc = heredoc;

		parse_interpolate(&state, false);

		heredoc->node->data = parser.parse_data(Lexeme::STRING);
	}
	
	bool Lexer::heredoc_terminates(Heredoc *heredoc)
	{
		const char_t *start = &input;
		size_t line = lexeme.line;
		const char_t *line_start = lexeme.line_start;

		if(input == '\n')
		{
			input++;
			line++;

			line_start = &input;
		}
		else if(input == '\r')
		{
			input++;
			line++;

			if(input == '\n')
				input++;

			line_start = &input;
		}
		
		if(heredoc->remove_ident)
			while(is_white())
				input++;
		
		const char_t *str = &input;

		while(input != '\n' && input != '\r' && (input != 0 || !process_null(&input)))
			input++;

		if((size_t)&input - (size_t)str != heredoc->name.length)
		{
			input.set(start);
			return false;
		}

		if(std::memcmp(str, heredoc->name.data, heredoc->name.length) == 0)
		{
			lexeme.line = line;
			lexeme.line_start = line_start;
			return true;
		}

		input.set(start);
		return false;
	}

	void Lexer::process_newline(bool no_heredoc)
	{
		lexeme.line++;
		lexeme.line_start = &input;

		if(heredocs.size() && !no_heredoc)
		{
			Heredoc *heredoc = heredocs.pop();
		
			Lexeme old = lexeme;

			Tree::Scope *current_scope = parser.scope;
			Tree::Fragment current_fragment = parser.fragment;
			Tree::VoidTrapper *current_trapper = parser.trapper;

			parser.fragment = heredoc->fragment;
			parser.scope = heredoc->scope;
			parser.trapper = heredoc->trapper;

			parse_interpolate_heredoc(heredoc);
				
			parser.scope = current_scope;
			parser.fragment = current_fragment;
			parser.trapper = current_trapper;
			
			if(input == '\n')
				newline();
			else if(input == '\r')
				carrige_return();

			lexeme = old;
		}
	}

	void Lexer::newline()
	{
		input++;
		lexeme.type = Lexeme::LINE;

		process_newline();
	
		lexeme.start = &input;
		lexeme.stop = &input;
	}

	void Lexer::carrige_return()
	{
		input++;
		lexeme.type = Lexeme::LINE;

		if(input == '\n')
			input++;

		process_newline();

		lexeme.start = &input;
		lexeme.stop = &input;
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
				parser.report(lexeme, "Instance variable names cannot start with a number");
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
					parser.report(lexeme, "Class variable names cannot start with a number");
			}
			else
			{
				lexeme.stop = &input;
				lexeme.type = Lexeme::CVAR;
				lexeme.symbol = symbol_pool.get(lexeme);

				parser.report(lexeme, "Expected a class variable name");
			}
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::IVAR;
			lexeme.symbol = symbol_pool.get(lexeme);

			parser.report(lexeme, "Expected a instance variable name");
		}
	}
	
	void Lexer::global()
	{
		input++;

		if(!is_start_ident(input) || !is_ident((&input)[1]))
		{
			switch(input)
			{
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
				case ':':
				case '>':
				case '\\':
				case '/':
				case '$':
				case '_':
				case '-':
				case '.':
				case ',':
				case '~':
				case ';':
				case '"':
				case '\'':
				case '*':
				case '?':
				case '!':
					input++;
					lexeme.stop = &input;
					lexeme.type = Lexeme::GLOBAL;
					lexeme.symbol = symbol_pool.get(lexeme);
					break;

				default:
					lexeme.stop = &input;
					lexeme.type = Lexeme::GLOBAL;
					lexeme.symbol = symbol_pool.get(lexeme);

					parser.report(lexeme, "Expected a global variable name");
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
			parser.report(lexeme, "Global variable names cannot start with a number");
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
