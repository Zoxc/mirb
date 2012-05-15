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
	
	class File:
		public IO
	{
		public:
			File() : IO(Value::File, context->file_class) {}

			template<typename F> void mark(F mark)
			{
				IO::mark(mark);
			}
			
			static bool absolute_path(const CharArray &path);
			static CharArray join(const CharArray &left, const CharArray &right);
			static bool fnmatch(const CharArray &path, const CharArray &pattern);
			static CharArray normalize_path(const CharArray &path);
			static CharArray basename(CharArray path);
			static value_t rb_expand_path(String *relative, String *absolute);
			static CharArray expand_path(CharArray relative);
			static CharArray expand_path(CharArray relative, CharArray from);

			static void initialize();
	};
};
