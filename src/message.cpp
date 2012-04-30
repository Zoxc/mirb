#include "message.hpp"
#include "document.hpp"
#include "parser/parser.hpp"
#include "generic/range.hpp"
#include "platform/platform.hpp"

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
	
	void Message::print()
	{
		Platform::color<Platform::Bold>(parser.document.name + "[" + CharArray::uint(range.line + 1) + "]: ");
		Platform::color<Platform::Red>(severity_names[severity]);
		Platform::color<Platform::Bold>(": " + string() + "\n");
		std::cerr << range.get_line() << "\n";
		Platform::color<Platform::Green>(range.indicator());
		std::cerr << std::endl;
	}

	std::string Message::format()
	{
		std::stringstream result;

		result << parser.document.name.get_string() << "[" << range.line + 1 << "]: " << severity_names[severity] << ": " << string() << std::endl << range.get_line() << std::endl;
		
		return result.str() + range.indicator().get_string();
	}
};
