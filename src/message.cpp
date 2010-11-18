#include "message.hpp"
#include "parser/parser.hpp"
#include "range.hpp"

namespace Mirb
{
	std::string Message::severity_names[SEVERITIES] = {
		"Hint",
		"Warning",
		"Error"
	};
	
	Message::Message(Parser &parser, Range &range, Severity severity) : parser(parser), range(range), severity(severity)
	{
		auto i = parser.messages.mutable_iterator();
		
		while(i)
		{
			if(i().range.start > range.start)
				break;
				
			++i;
		}
		
		i.insert(this);
	}
	
	std::string Message::format()
	{
		std::stringstream result;

		result << parser.filename << "[" << range.line + 1 << "]: " << severity_names[severity] << ": " << string() << std::endl << range.get_line() << std::endl;
		
		for(const char_t *i = range.line_start; i < range.start; ++i)
		{
			if(*i == '\t')
				result << '\t';
			else
				result << " ";
		}
		
		if(range.length() <= 1)
			result << "^";
		else
		{
			for(const char_t *i = range.start; i < range.stop; ++i)
			{
				switch(*i)
				{
					case 0:
					case 13:
					case 10:
						goto done;
						
					default:
						result << "~";	
				}
			}
		}
			
		done:
		return result.str();
	}
};
