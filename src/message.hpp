#pragma once
#include "common.hpp"
#include "generic/source-loc.hpp"
#include <Prelude/List.hpp>

namespace Mirb
{
	class Parser;
	class Stream;

	class Message
	{
		public:
			enum Severity
			{
				MESSAGE_HINT,
				MESSAGE_WARNING,
				MESSAGE_ERROR,
				MESSAGE_NOTE,
				SEVERITIES
			};

			Message(Parser &parser, const SourceLoc &range, Severity severity, const CharArray &text);

			~Message()
			{
				if(note)
					note->~Message();
			}

			static std::string severity_names[SEVERITIES];

			Document *document;
			SourceLoc range;
			Severity severity;
			Message *note;
			CharArray message;

			ListEntry<Message> entry;
			 
			template<typename F> void mark(F mark)
			{
				mark(document);
				mark(message);

				if(note)
					note->mark(mark);
			}

			void print(Stream &out);
	};
};
