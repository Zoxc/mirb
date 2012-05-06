#pragma once
#include "io.hpp"
#include "../context.hpp"

namespace Mirb
{
	struct JoinSegments
	{
		std::vector<CharArray> segments;
		
		void push(const CharArray &path);
		CharArray join();
	};

	CharArray basename(CharArray path);

	class File:
		public IO
	{
		public:
			File() : IO(Value::File, context->file_class) {}

			template<typename F> void mark(F mark)
			{
				IO::mark(mark);
			}

			static void initialize();
	};
};
