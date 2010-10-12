#pragma once
#include "memory_pool.hpp"

namespace Mirb
{
	class Range
	{
		public:
			const char_t *start;
			const char_t *stop;
			const char_t *line_start;
			size_t line;
			bool error;
			
			Range() : start(0), stop(0), error(false) {}
			
			void capture(const Range &range)
			{
				start = range.start;
				stop = range.stop;
				line_start = range.line_start;
				line = range.line;
				error = range.error;
			}
			
			Range(const Range &range) : start(0), stop(0), error(false)
			{
				capture(range);
			}
			
			void expand(Range &range)
			{
				stop = range.stop;
			}
			
			Range &dup(MemoryPool &memory_pool)
			{
				return *new (memory_pool) Range(*this);
			}
			
			std::string string()
			{
				std::string result((const char *)start, length());

				return result;
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
			
			size_t length()
			{
				return stop - start;
			}
	};
};
