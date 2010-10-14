#pragma once
#include "common.hpp"
#include "memory-pool.hpp"
#include "message.hpp"
#include "simple-list.hpp"
#include "parser/parser.hpp"

namespace Mirb
{
	class Compiler
	{
		private:
		public:
			MemoryPool memory_pool;
			SimpleList<Message, Message, &Message::entry> messages;
			Parser parser;
			const char *filename;
			
			void load(const char_t *input, size_t length);
			
			void report(Range &range, std::string text, Message::Severity severity = Message::MESSAGE_ERROR);
			
			Compiler();
			~Compiler();
	};
};
