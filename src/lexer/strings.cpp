#include "lexer.hpp"
#include "../symbol-pool.hpp"
#include "../parser/parser.hpp"

namespace Mirb
{
	void Lexer::build_simple_string(const char_t *start, char_t *str, size_t length prelude_unused)
	{
		char_t *writer = str;
		const char_t *input = start;
	 
		while(true)
			switch(*input)
			{
				case '\'':
					goto done;
					
				case '\\':
					input++;
					
					switch(*input)
					{
						case '\'':
						case '\\':
							*writer++ = *input++;
							break;
						
						case 0:
							if(process_null(input, true))
								goto done;

						default:
							*writer++ = '\\';
							*writer++ = *input++;
					}
					break;
				
				case 0:
					if(process_null(input, true))
						goto done;
				
				default:
					*writer++ = *input++;
			}
			
		done:
		mirb_debug_assert((size_t)(writer - str) == length);
	}

	void Lexer::simple_string()
	{
		input++;
		
		lexeme.type = Lexeme::STRING;
		size_t overhead = 2;

		while(true)
			switch(input)
			{
				case '\'':
					input++;
					goto done;

				case '\n':
					input++;
					lexeme.line++;
					lexeme.line_start = &input;
					break;
						
				case '\r':
					input++;
					lexeme.line++;

					if(input == '\n')
						input++;

					lexeme.line_start = &input;
					break;

				case '\\':
					input++;

					switch(input)
					{
						case '\'':
						case '\\':
							overhead++;
							input++;
							break;
						
						case 0:
							if(process_null(&input))
							{
								lexeme.stop = &input;
								parser.report(lexeme.dup(memory_pool), "Unterminated string");
								goto error;
							}
							else
								input++;
							break;
						
						default:
							const char_t *start = lexeme.start;
							lexeme.start = &input - 1;
							lexeme.stop = &input + 1;
							parser.report(lexeme.dup(memory_pool), "Invalid escape string");
							lexeme.start = start;
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
					input++;		
			}

		error:
		lexeme.stop = &input;
		lexeme.str = new (memory_pool) StringData(memory_pool);
		return;
		
		done:
		lexeme.stop = &input;
		
		size_t str_length = lexeme.length() - overhead;
		char_t *str = new (memory_pool) char_t[str_length];
		
		build_simple_string(lexeme.start + 1, str, str_length);
		
		lexeme.str = new (memory_pool) StringData(memory_pool);
		lexeme.str->tail.data = str;
		lexeme.str->tail.length = str_length;
	}
	
	void Lexer::parse_string(InterpolatedState *state)
	{
		lexeme.type = Lexeme::STRING;
		std::string result;
		StringData *data = new (memory_pool) StringData(memory_pool);
		
		const char_t *start = lexeme.start;

		auto push = [&] {
			auto entry = new (memory_pool) StringData::AdvancedEntry;
			entry->set<MemoryPool>(result, memory_pool);
			entry->type = lexeme.type;
			entry->symbol = lexeme.symbol;
			result = "";
			data->entries.push(entry);
		};

		lexeme.type = state ? Lexeme::STRING_END : Lexeme::STRING;
		
		while(true)
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
								lexeme.type = state ? Lexeme::STRING_CONTINUE : Lexeme::STRING_START;

								if(!state)
								{
									state = new (memory_pool) InterpolatedState;

									lexeme.start = start;
									lexeme.stop = &input;

									state->start = new (memory_pool) Range(lexeme);
								}

								lexeme.curlies.push(state);
							}
							goto done;

						case '@':
							{
								lexeme.start = &input;
								Lexeme::Type old_type = lexeme.type;

								ivar(false);

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

				case '"':
					input++;
					goto done;

				case '\n':
					result += input++;
					lexeme.line++;
					lexeme.line_start = &input;
					break;
						
				case '\r':
					result += input++;
					lexeme.line++;

					if(input == '\n')
						result += input++;

					lexeme.line_start = &input;
					break;

				case '\\':
					input++;

					switch(input)
					{
						case '\'':
						case '\"':
						case '\\':
							result += input++;
							break;
							
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
							{
								lexeme.stop = &input;

								parser.report(lexeme.dup(memory_pool), "Unterminated string");
								goto error;
							}

							// Fallthrough
							
						default:
							const char_t *start = lexeme.start;
							lexeme.start = &input - 1;
							lexeme.stop = &input + 1;
							parser.report(lexeme.dup(memory_pool), "Invalid escape string");
							lexeme.start = start;
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
		lexeme.str = new (memory_pool) StringData(memory_pool);
		return;
		
		done:
		lexeme.stop = &input;
		
		lexeme.str = data;

		data->tail.set<MemoryPool>(result, memory_pool);
	}
	
	void Lexer::curly_close()
	{
		input++;
		lexeme.stop = &input;
		lexeme.type = Lexeme::CURLY_CLOSE;
		
		if(lexeme.curlies.size() == 0)
			return;
		
		InterpolatedState *state = lexeme.curlies.pop();
		
		if(!state)
			return;
		
		parse_string(state);
	}

	void Lexer::string()
	{
		input++;
		
		parse_string(nullptr);
	}
};
