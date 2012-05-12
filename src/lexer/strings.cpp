#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	void Lexer::simple_string()
	{
		input++;
		
		lexeme.type = Lexeme::STRING;
		std::string result;

		while(true)
			switch(input)
			{
				case '\'':
					input++;
					goto done;

				case '\n':
					result += input++;
					process_newline();
					break;
						
				case '\r':
					result += input++;

					if(input == '\n')
						result += input++;
					
					process_newline();
					break;

				case '\\':
					input++;

					switch(input)
					{
						case '\'':
						case '\\':
							result += input++;
							break;
						
						case 0:
							if(process_null(&input))
							{
								lexeme.stop = &input;
								parser.report(lexeme.dup(memory_pool), "Unterminated string");
								goto error;
							}

							// Fallthrough

						default:
							result += input++;
					}
					break;

				case 0:
					if(process_null(&input))
					{
						lexeme.stop = &input;
						parser.report(lexeme.dup(memory_pool), "Unterminated string");
						goto error;
					}
				
					// Fallthrough

				default:
					result += input++;		
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
				result += (char)0;
				input++;
				break;
						
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
		
		auto push = [&] {
			auto entry = new (memory_pool) InterpolateData::AdvancedEntry;
			entry->set<MemoryPool>(result, memory_pool);
			entry->type = lexeme.type;
			entry->symbol = lexeme.symbol;
			result = "";
			lexeme.data->entries.push(entry);
		};

		auto report_end = [&] {
			lexeme.stop = &input;

			if(state->start)
			{
				auto message = new (memory_pool) StringMessage(parser, lexeme.dup(memory_pool), Message::MESSAGE_ERROR, "Unterminated interpolated " + Lexeme::describe_type(state->type));
			
				message->note = new (memory_pool) StringMessage(parser, *state->start, Message::MESSAGE_NOTE, "Starting here");
			
				parser.add_message(message, lexeme);
			}
			else
				parser.report(lexeme.dup(memory_pool), "Unterminated " + Lexeme::describe_type(state->type));
		};

		while(true)
		{
			if(input == state->terminator && !state->heredoc)
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

								global();

								if(lexeme.type != Lexeme::NONE)
									push();

								lexeme.start = start;
								lexeme.type = old_type;
							}
							break;

						case '@':
							{
								lexeme.start = &input;
								Lexeme::Type old_type = lexeme.type;

								ivar();

								if(lexeme.type != Lexeme::NONE)
									push();

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
					process_newline(state->heredoc != 0);
					break;
						
				case '\r':
					if(state->heredoc && heredoc_terminates(state->heredoc))
						goto done;

					result += input++;

					if(input == '\n')
						result += input++;
					
					process_newline(state->heredoc != 0);
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
		
		InterpolateState state;

		state.terminator = '`';
		state.type = Lexeme::COMMAND;

		parse_interpolate(&state, false);
	}
};
