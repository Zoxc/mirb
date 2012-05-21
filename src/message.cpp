#include "message.hpp"
#include "document.hpp"
#include "parser/parser.hpp"
#include "generic/source-loc.hpp"
#include "platform/platform.hpp"
#include "classes/file.hpp"

namespace Mirb
{
	std::string Message::severity_names[SEVERITIES] = {
		"Hint",
		"Warning",
		"Error",
		"Note"
	};
	
	Message::Message(Parser &parser, const SourceLoc &range, Severity severity, std::string text) : parser(parser), range(range), severity(severity), note(nullptr), text(text)
	{
	}
	
	void Message::print()
	{
		CharArray line_prefix = (const char_t *)(severity == MESSAGE_NOTE ? "  " : "");

		std::cerr << line_prefix.get_string();
		switch(severity)
		{
			case MESSAGE_ERROR:
				Platform::color<Platform::Red>(severity_names[severity]);
				break;

			case MESSAGE_NOTE:
				Platform::color<Platform::Blue>(severity_names[severity]);
				break;

			default:
				std::cerr << severity_names[severity];
		}

		Platform::color<Platform::Bold>(": " + string() + "\n");

		CharArray prefix = line_prefix + File::basename(parser.document.name) + "[" + CharArray::uint(range.line + 1) + "]: ";
		
		Platform::color<Platform::Bold>(prefix);

		std::cerr << range.get_line() << "\n";
		Platform::color<Platform::Green>(CharArray(" ") * prefix.size() + range.indicator());
		std::cerr << std::endl;

		if(note)
			note->print();
	}

	std::string Message::format()
	{
		std::stringstream result;

		result << parser.document.name.get_string() << "[" << range.line + 1 << "]: " << severity_names[severity] << ": " << string() << std::endl << range.get_line() << std::endl;
		
		return result.str() + range.indicator().get_string();
	}
};
