#pragma once
#include "common.hpp"
#include "memory-pool.hpp"
#include "message.hpp"
#include "simple-list.hpp"

namespace Mirb
{
	class Compiler
	{
		private:
		public:
			MemoryPool memory_pool;
			SimpleList<Message, Message, &Message::entry> messages;
			const char *filename;
			
			void report(Range &range, std::string text, Message::Severity severity = Message::MESSAGE_ERROR);
			
			Compiler();
			~Compiler();
	};
};
