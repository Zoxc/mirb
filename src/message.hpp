#pragma once
#include "common.hpp"
#include <Prelude/List.hpp>

namespace Mirb
{
	class Parser;
	class Range;

	class Message
	{
		public:
			enum Severity
			{
				MESSAGE_HINT,
				MESSAGE_WARNING,
				MESSAGE_ERROR,
				SEVERITIES
			};

			Message(Parser &parser, Range &range, Severity severity);

			static std::string severity_names[SEVERITIES];

			Parser &parser;
			Range &range;
			Severity severity;
			ListEntry<Message> entry;

			virtual std::string string() = 0;
			
			std::string format();
			void print();
	};
	
	class StringMessage:
		public Message
	{
		private:
			std::string text;
		public:
			StringMessage(Parser &parser, Range &range, Severity severity, std::string text) : Message(parser, range, severity), text(text) {}

			std::string string()
			{
				return text;
			}
	};
};
