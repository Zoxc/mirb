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
	
	bool Lexer::whitespace_after()
	{
		return input.in(1, 32);
	}

	void Lexer::unknown()
	{
		input++;
		
		while(jump_table[input] == &Lexer::unknown)
			input++;

		lexeme.stop = &input;
		lexeme.type = Lexeme::NONE;

		if (lexeme.length() == 1)
			report(lexeme, "Invalid character '" + lexeme.string() + "'");
		else
			report(lexeme, "Invalid characters '" + lexeme.string() + "'");

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

	void Lexer::expand_to_unary()
	{
		if(input == '@')
		{
			input++;
			lexeme.stop = &input;
			lexeme.type = Lexeme::operator_to_unary(lexeme.type);
		}
	}

	void Lexer::parse_delimited_data(Lexeme::Type type)
	{
		InterpolateState state;
		state.type = type;

		state.opener = input;
		state.terminator = delimiter_mapping.try_get(input, [&]{
			return input;
		});

		if(input != '\0' || !process_null(&input, true))
			input++;

		parse_interpolate(&state, false);
	}

	void Lexer::left_shift_to_heredoc()
	{
		auto heredoc = new (memory_pool) Heredoc;
		
		heredoc->scope = parser.scope;

		if(input == '-')
		{
			input++;
			heredoc->remove_ident = true;
		}
		else
			heredoc->remove_ident = false;
		
		CharArray name;

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
					report(lexeme, "Expected heredoc delimiter");

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
						report(lexeme, "Unterminated heredoc delimiter");
						return;
					}

					name += input++;
				}

				input++;
			};

			switch(input)
			{
				case '`':
				case '"':
				case '\'':
					get_name();
					break;

				default:
					lexeme.stop = &input;
					report(lexeme, "Expected heredoc delimiter");
					return;
			}
		}

		heredoc->name.set<MemoryPool>(name, memory_pool);

		heredocs.push(heredoc);
		
		heredoc->node = new (Tree::Fragment(*heredoc->scope->fragment)) Tree::HeredocNode;

		lexeme.stop = &input;

		heredoc->range = lexeme;

		lexeme.type = Lexeme::HEREDOC;
		lexeme.heredoc = heredoc->node;
	}
	
	void Lexer::mod_to_literal()
	{
		input.set(lexeme.start + 1);

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
				
			case 'q':
				{
					input++;
				
					char_t opener = input;
					char_t terminator = delimiter_mapping.try_get(input, [&]{
						return input;
					});

					input++;

					return parse_simple_string(opener, terminator, opener != terminator);
				}
				
			case 'w':
			{
				input++;
				
				char_t opener = input;
				char_t terminator = delimiter_mapping.try_get(input, [&]{
					return input;
				});

				if(input == 0 && process_null(&input))
					return;

				input++;

				CharArray result;
				size_t nested = 0;

				while(input != terminator || nested > 0)
				{
					if(opener != terminator)
					{
						if(input == opener)
							nested++;
						else if(input == terminator)
							nested--;
					}

					switch(input)
					{
						case '\n':
							result += input++;
							process_newline(false, false);
							break;
						
						case '\r':
							result += input++;

							if(input == '\n')
								result += input++;
					
							process_newline(false, false);
							break;

						case 0:
							if(process_null(&input))
							{
								lexeme.stop = &input;
								report(lexeme, "Unterminated array literal");
								goto exit_array_literal;
								return;
							}
				
							// Fallthrough

						default:
							result += input++;	

					}
				}
				
				input++;

			exit_array_literal:
				lexeme.stop = &input;
				lexeme.data = new (memory_pool) InterpolateData(memory_pool);
				lexeme.data->tail.set<MemoryPool>(result, memory_pool);
				lexeme.type = Lexeme::ARRAY;
				
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

	void Lexer::regexp_options()
	{
		while(is_char(input))
		{
			switch(input)
			{
				case 'i':
				case 'm':
				case 'u':
				case 's':
				case 'n':
				case 'o':
				case 'e':
				case 'x':
					break;

				default:
					report(range(&input, &input + 1), "Unknown regular expression option '" + CharArray(&input, 1) + "'");
			}

			input++;
		}
	}

	void Lexer::div_to_regexp()
	{
		input.set(lexeme.start + 1);

		InterpolateState state;
		state.type = Lexeme::REGEXP;
		state.terminator = state.opener = '/';
		parse_interpolate(&state, false);
	}
	
	void Lexer::question_to_character()
	{
		CharArray result;

		if(input == '\\')
		{
			input++;

			bool dummy;

			if(!parse_escape(result, true, false, dummy))
			{
				if(input == 0 && process_null(&input))
				{
					lexeme.stop = &input;
					lexeme.data = new (memory_pool) InterpolateData(memory_pool);
					lexeme.type = Lexeme::STRING;
					report(lexeme, "Expected escape string");

					return;
				}
			}
			else
				goto result;
		}
		else if(input == 0)
		{
			if(process_null(&input))
			{
				lexeme.stop = &input;
				lexeme.data = new (memory_pool) InterpolateData(memory_pool);
				lexeme.type = Lexeme::STRING;
				report(lexeme, "Expected escape string");

				return;
			}
		}

		result = input++;

	result:
		lexeme.data = new (memory_pool) InterpolateData(memory_pool);
		lexeme.data->tail.set<MemoryPool>(result, memory_pool);
		lexeme.stop = &input;
		lexeme.type = Lexeme::STRING;
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
		state.type = heredoc->type == '`' ? Lexeme::COMMAND : Lexeme::STRING;
		state.heredoc = heredoc;

		parse_interpolate(&state, false);

		heredoc->node->data = parser.parse_data(Lexeme::STRING);
	}
	
	bool Lexer::heredoc_terminates(Heredoc *heredoc)
	{
		const char_t *start = &input;
		size_t line = lexeme.current_line;
		const char_t *line_start = lexeme.current_line_start;

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
			lexeme.current_line = line;
			lexeme.current_line_start = line_start;
			return true;
		}

		input.set(start);
		return false;
	}
	
	void Lexer::process_comment()
	{
		if((std::strncmp((const char *)&input, "=begin", 6) == 0) && !is_ident((&input)[6]))
		{
			auto terminate = [&]() -> bool {
				if((std::strncmp((const char *)&input, "=end", 4) == 0) && !is_ident((&input)[4]))
				{
					input.set(&input + 4);
					return true;
				}
				else
					return false;
			};

			while(true)
			{
				switch(input)
				{
					case '\r':
					{
						if(input == '\n')
							input++;
						
						lexeme.current_line++;
						lexeme.current_line_start = &input;

						if(terminate())
							goto exit_comment;

						break;
					}
					
					case '\n':
					{
						input++;
						
						lexeme.current_line++;
						lexeme.current_line_start = &input;

						if(terminate())
							goto exit_comment;

						break;
					}

					case 0:
						if(process_null(&input))
						{
							lexeme.stop = &input;
							report(lexeme, "Unterminated multi-line comment");
							goto exit_comment;
						}
						break;

					default:
						input++;
						break;
				}
			}
			exit_comment:;
		}
	}

	void Lexer::process_newline(bool no_heredoc, bool allow_comment)
	{
		lexeme.current_line++;
		lexeme.current_line_start = &input;

		if(heredocs.size() && !no_heredoc)
		{
			Heredoc *heredoc = heredocs.pop();
		
			Lexeme old = lexeme;
			
			lexeme.start = &input;
			lexeme.line = lexeme.current_line;
			lexeme.line_start = lexeme.current_line_start;

			parser.enter_scope(heredoc->scope, [&] {
				parse_interpolate_heredoc(heredoc);
			});
			
			old.current_line = lexeme.current_line;
			old.current_line_start = lexeme.current_line_start;
			lexeme = old;

			if(input == '\n')
				newline();
			else if(input == '\r')
				carrige_return();

		}

		if(allow_comment)
			process_comment();
	}

	void Lexer::newline()
	{
		input++;
		lexeme.type = Lexeme::LINE;

		process_newline(false, true);
	
		lexeme.start = &input;
		lexeme.stop = &input;
	}

	void Lexer::carrige_return()
	{
		input++;
		lexeme.type = Lexeme::LINE;

		if(input == '\n')
			input++;

		process_newline(false, true);

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
	
	bool Lexer::is_char(char_t c)
	{
		return Input::char_in(c, 'a', 'z') || Input::char_in(c, 'A', 'Z');
	}
	
	bool Lexer::is_alpha(char_t c)
	{
		return is_char(c) || Input::char_in(c, '0', '9');
	}
	
	bool Lexer::is_start_ident(char_t c)
	{
		return is_char(c) || c == '_';
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
				report(lexeme, "Instance variable names cannot start with a number");
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
					report(lexeme, "Class variable names cannot start with a number");
			}
			else
			{
				lexeme.stop = &input;
				lexeme.type = Lexeme::CVAR;
				lexeme.symbol = symbol_pool.get(lexeme);

				report(lexeme, "Expected a class variable name");
			}
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::IVAR;
			lexeme.symbol = symbol_pool.get(lexeme);

			report(lexeme, "Expected a instance variable name");
		}
	}
	
	void Lexer::global()
	{
		input++;

		if(!is_ident(input))
		{
			switch(input)
			{
				case '-':
					{
						input++;

						if(is_ident(input))
							input++;

						lexeme.stop = &input;
						lexeme.type = Lexeme::GLOBAL;
						lexeme.symbol = symbol_pool.get(lexeme);
						break;
					};

				case ':':
				case '>':
				case '<':
				case '\\':
				case '/':
				case '@':
				case '$':
				case '`':
				case '&':
				case '_':
				case '+':
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

					report(lexeme, "Expected a global variable name");
					break;
			}

			return;
		}

		skip_ident();
			
		lexeme.stop = &input;
		lexeme.type = Lexeme::GLOBAL;
		lexeme.symbol = symbol_pool.get(lexeme);
	}
	
	void Lexer::sub()
	{
		input++;

		switch(input)
		{
			case '=':
			{
				input++;
				lexeme.type = Lexeme::ASSIGN_SUB;
				break;
			}
			
			case '>':
			{
				input++;
				lexeme.type = Lexeme::BLOCK_POINT;
				break;
			}

			default:
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
