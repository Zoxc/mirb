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
	
	Message::Message(Parser &parser, const SourceLoc &range, Severity severity, std::string text) : document(&parser.document), range(range), severity(severity), note(nullptr), message(text)
	{
	}
	
	void Message::print(Stream &out)
	{
		CharArray line_prefix = (const char_t *)(severity == MESSAGE_NOTE ? "  " : "");

		out.print(line_prefix);

		switch(severity)
		{
			case MESSAGE_ERROR:
				out.color(Red, severity_names[severity]);
				break;

			case MESSAGE_NOTE:
				out.color(Blue, severity_names[severity]);
				break;

			default:
				out.print(severity_names[severity]);
		}

		out.color(Bold, ": " + message + "\n");

		CharArray prefix = line_prefix + File::basename(document->name) + "[" + CharArray::uint(range.line + 1) + "]: ";
		
		out.color(Bold, prefix);

		out.print(range.get_line() + "\n");
		out.color(Green, CharArray(" ") * prefix.size() + range.indicator());

		if(note)
			note->print(out);
	}
};
