#pragma once
#include "memory-pool.hpp"
#include "../char-array.hpp"

namespace Mirb
{
	class BasicRange
	{
		public:
			const char_t *start;
			const char_t *stop;

			BasicRange() : start(0), stop(0) {}
			
			void expand(BasicRange &range)
			{
				stop = range.stop;
			}
			
			std::string string()
			{
				std::string result((const char *)start, length());

				return result;
			}
			
			size_t length()
			{
				return stop - start;
			}
	};
	
	class Range:
		public BasicRange
	{
		public:
			const char_t *line_start;
			size_t line;

			Range &dup(MemoryPool &memory_pool)
			{
				return *new (memory_pool) Range(*this);
			}
	
			std::string get_line()
			{
				const char_t *input = line_start;
				
				while(input)
					switch(*input)
					{
						case 0:
						case 13:
						case 10:
							goto done;
							
						default:
							input++;	
					}
					
				done:
					
				std::string result((const char *)line_start, (size_t)(input - line_start));
						
				return result;
			}
	
			CharArray indicator()
			{
				CharArray result;

				for(const char_t *i = line_start; i < start; ++i)
				{
					if(*i == '\t')
						result += "\t";
					else
						result += " ";
				}
		
				if(length() <= 1)
					result += "^";
				else
				{
					for(const char_t *i = start; i < stop; ++i)
					{
						switch(*i)
						{
							case 0:
							case 13:
							case 10:
								goto done;
						
							default:
								result += "~";	
						}
					}
				}
			
				done:
				return result;
			}
	};
};
