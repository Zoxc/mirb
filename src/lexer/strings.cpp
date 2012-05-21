#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	void Lexer::simple_string()
	{
		input++;
		
		parse_simple_string('\'');
	}

	void Lexer::parse_simple_string(char_t terminator)
	{
		lexeme.type = Lexeme::STRING;
		std::string result;

		while(true)
		{
			if(input == terminator)
			{
				input++;
				goto done;
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

				case '\\':
					input++;

					if(input == '\\' || input == terminator)
						result += input++;

					break;

				case 0:
					if(process_null(&input))
					{
						lexeme.stop = &input;
						report(lexeme, "Unterminated string");
						goto error;
					}
				
					// Fallthrough

				default:
					result += input++;		
			}
		}

		error:
		lexeme.stop = &input;
		lexeme.data = new (memory_pool) InterpolateData(memory_pool);
		return;
		
		done:
		lexeme.stop = &input;
		
		lexeme.data = new (memory_pool) InterpolateData(memory_pool);
		lexeme.data->tail.set<MemoryPool>(result, memory_pool);
	}
	
	bool Lexer::parse_escape(std::string &result)
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
				{
					const char_t *start = &input;

					input++;
				
					while(input.in('0', '7'))
						input++;

					size_t num = 0;
					size_t power = 0;

					for(const char_t *c = &input - 1; c >= start; --c, power += 3)
						num += (*c - '0') << power;

					result += (char)num;
					break;
				}
						
			case 'n':
				result += (char)0xA;
				input++;
				break;
						
			case 't':
				result += (char)0x9;
				input++;
				break;
						
			case 'r':
				result += (char)0xD;
				input++;
				break;
						
			case 'f':
				result += (char)0xC;
				input++;
				break;
						
			case 'v':
				result += (char)0xB;
				input++;
				break;
						
			case 'a':
				result += (char)0x7;
				input++;
				break;
						
			case 'e':
				result += (char)0x1B;
				input++;
				break;
						
			case 'b':
				result += (char)0x8;
				input++;
				break;
						
			case 's':
				result += (char)0x20;
				input++;
				break;
						
			case 0: 
				if(process_null(&input))
					return true;
			
			default:
				result += input++;
		}

		return false;
	}

	void Lexer::parse_interpolate(InterpolateState *state, bool continuing)
	{
		lexeme.type = state->type;
		std::string result;

		lexeme.data = new (memory_pool) InterpolateData(memory_pool);

		if(continuing)
			lexeme.data->type = InterpolateData::Ending;
		
		const char_t *start = lexeme.start;
		
		auto push = [&](InterpolateData *data) {
			auto entry = new (memory_pool) InterpolateData::AdvancedEntry;
			entry->set<MemoryPool>(result, memory_pool);
			entry->type = lexeme.type;
			entry->symbol = lexeme.symbol;
			result = "";
			data->entries.push(entry);
		};

		auto report_end = [&] {
			lexeme.stop = &input;

			if(state->start)
			{
				auto message = new (memory_pool) Message(parser, lexeme, Message::MESSAGE_ERROR, "Unterminated interpolated " + Lexeme::describe_type(state->type));
			
				message->note = new (memory_pool) Message(parser, *state->start, Message::MESSAGE_NOTE, "Starting here");
			
				parser.add_message(message, lexeme);
			}
			else
				report(lexeme, "Unterminated " + Lexeme::describe_type(state->type));
		};

		while(true)
		{
			if(!state->heredoc && input == state->terminator)
			{
				input++;
				goto done;
			}

			switch(input)
			{
				case '#':
				{
					input++;

					switch(input)
					{
						case '{':
							input++;

							{
								if(!continuing)
								{
									lexeme.data->type = InterpolateData::Starting;

									state = new (memory_pool) InterpolateState(*state);

									lexeme.start = start;
									lexeme.stop = &input;

									state->start = new (memory_pool) SourceLoc(lexeme);
								}
								else
									lexeme.data->type = InterpolateData::Continuing;

								lexeme.curlies.push(state);
							}
							goto done;
							
						case '$':
							{
								lexeme.start = &input;
								Lexeme::Type old_type = lexeme.type;
								InterpolateData *data = lexeme.data;

								global();

								if(lexeme.type != Lexeme::NONE)
									push(data);
								
								lexeme.data = data;
								lexeme.start = start;
								lexeme.type = old_type;
							}
							break;

						case '@':
							{
								lexeme.start = &input;
								Lexeme::Type old_type = lexeme.type;
								InterpolateData *data = lexeme.data;

								ivar();

								if(lexeme.type != Lexeme::NONE)
									push(data);
								
								lexeme.data = data;
								lexeme.start = start;
								lexeme.type = old_type;
							}
							break;

						default:
							result += '#';
							break;
					}
					break;
				}

				case '\n':
					if(state->heredoc && heredoc_terminates(state->heredoc))
						goto done;

					result += input++;
					process_newline(state->heredoc != 0, false);
					break;
						
				case '\r':
					if(state->heredoc && heredoc_terminates(state->heredoc))
						goto done;

					result += input++;

					if(input == '\n')
						result += input++;
					
					process_newline(state->heredoc != 0, false);
					break;

				case '\\':
					input++;

					if(parse_escape(result))
						return report_end();
					break;

				case 0:
					if(process_null(&input))
						return report_end();

					// Fallthrough
				
				default:
					result += input++;
			}
		}
		
		done:
		lexeme.stop = &input;
		lexeme.data->tail.set<MemoryPool>(result, memory_pool);

		if(lexeme.data->type == InterpolateData::Plain || lexeme.data->type == InterpolateData::Ending)
		{
			if(state->type == Lexeme::REGEXP)
				regexp_options();
		}
	}
	
	void Lexer::curly_close()
	{
		input++;
		lexeme.stop = &input;
		lexeme.type = Lexeme::CURLY_CLOSE;
		
		if(lexeme.curlies.size() == 0)
			return;
		
		InterpolateState *state = lexeme.curlies.pop();
		
		if(!state)
			return;
		
		parse_interpolate(state, true);
	}

	void Lexer::string()
	{
		input++;
		
		InterpolateState state;

		state.terminator = '"';
		state.type = Lexeme::STRING;

		parse_interpolate(&state, false);
	}

	void Lexer::command()
	{
		input++;

		if(lexeme.allow_keywords)
		{
			InterpolateState state;

			state.terminator = '`';
			state.type = Lexeme::COMMAND;

			parse_interpolate(&state, false);
		}
		else
		{
			lexeme.stop = &input;
			lexeme.type = Lexeme::BACKTICK;
		}
	}
};
