#pragma once
#include "io.hpp"
#include "../context.hpp"

namespace Mirb
{
	namespace Platform
	{
		struct error_t;
	};

	struct JoinSegments
	{
		std::vector<CharArray> segments;
		
		void push(const CharArray &path);
		CharArray join();
	};
	
	namespace File
	{
		bool absolute_path(const CharArray &path);
		CharArray join(const CharArray &left, const CharArray &right);
		bool fnmatch(const CharArray &path, const CharArray &pattern);
		CharArray normalize_path(const CharArray &path);
		CharArray basename(CharArray path);
		CharArray dirname(CharArray path);
		value_t rb_expand_path(String *relative, String *absolute);
		CharArray expand_path(CharArray relative);
		CharArray expand_path(CharArray relative, CharArray from);
		
		IO *open(String *path, String *mode);

		void initialize();
	};
};
