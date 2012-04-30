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
				
				default:
					input++;		
			}

		error:
		lexeme.stop = &input;
		lexeme.str.data = (const char_t *)"";
		lexeme.str.length = 0;
		return;
		
		done:
		lexeme.stop = &input;
		
		size_t str_length = lexeme.length() - overhead;
		char_t *str = new (memory_pool) char_t[str_length];
		
		build_simple_string(lexeme.start + 1, str, str_length);
		
		lexeme.str.data = str;
		lexeme.str.length = str_length;
	}
	
	void Lexer::build_string(const char_t *start, char_t *str, size_t length prelude_unused)
	{
		char_t *writer = str;
		const char_t *input = start;
		
		while(true)
			switch(*input)
			{
				case '#':
				{
					input++;
					
					if(*input == '{')
					{
						goto done;
					}
					else
						*writer++ = '#';
					
					break;
				}
				
				case '"':
					goto done;
				
				case '\\':
					input++;

					switch(*input)
					{
						case '\'':
						case '\"':
						case '\\':
							*writer++ = *input++;
							break;
						
						case '0':
							*writer++ = 0;
							input++;
							break;
						
						case 'n':
							*writer++ = 0xA;
							input++;
							break;
						
						case 't':
							*writer++ = 0x9;
							input++;
							break;
						
						case 'r':
							*writer++ = 0xD;
							input++;
							break;
						
						case 'f':
							*writer++ = 0xC;
							input++;
							break;
						
						case 'v':
							*writer++ = 0xB;
							input++;
							break;
						
						case 'a':
							*writer++ = 0x7;
							input++;
							break;
						
						case 'e':
							*writer++ = 0x1B;
							input++;
							break;
						
						case 'b':
							*writer++ = 0x8;
							input++;
							break;
						
						case 's':
							*writer++ = 0x20;
							input++;
							break;
						
						case 0:
							if(process_null(input))
								goto done;

						default:
							*writer++ = '\\';
							*writer++ = *input++;
					}
					break;
					
				case 0:
					if(process_null(input))
						goto done;

				default:
					*writer++ = *input++;
			}
			
		done:
		mirb_debug_assert((size_t)(writer - str) == length);
	}

	void Lexer::parse_string(bool initial)
	{
		lexeme.type = Lexeme::STRING;
		size_t overhead = 2;
		
		while(true)
			switch(input)
			{
				case '#':
				{
					input++;
					
					if(input == '{')
					{
						input++;
						
						lexeme.curlies.push(true);
						
						lexeme.type = Lexeme::STRING_START;
						
						overhead += 1;

						goto done;
					}
					break;
				}

				case '"':
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
						case '\"':
						case '\\':
						case '0':
						case 'n':
						case 't':
						case 'r':
						case 'f':
						case 'v':
						case 'a':
						case 'e':
						case 'b':
						case 's':
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
				
				default:
					input++;		
			}
		
		error:
		lexeme.stop = &input;
		lexeme.str.data = (const char_t *)"";
		lexeme.str.length = 0;
		return;
		
		done:
		lexeme.stop = &input;
		
		if(!initial)
			lexeme.type = (Lexeme::Type)((int)lexeme.type + 1);
		
		size_t str_length = lexeme.length() - overhead;
		char_t *str = new (memory_pool) char_t[str_length + 1]; //TODO: Fix memory leak
		
		build_string(lexeme.start + 1, str, str_length);
		
		lexeme.str.data = str;
		lexeme.str.length = str_length;
	}
	
	void Lexer::curly_close()
	{
		input++;
		lexeme.stop = &input;
		lexeme.type = Lexeme::CURLY_CLOSE;
		
		if(lexeme.curlies.size() == 0)
			return;
		
		bool string = lexeme.curlies.pop();
		
		if(!string)
			return;
		
		parse_string(false);
	}

	void Lexer::string()
	{
		input++;
		
		parse_string(true);
	}
};
