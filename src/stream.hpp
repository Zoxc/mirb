#pragma once
#include "on-stack.hpp"

namespace Mirb
{
	enum Color
	{
		Green,
		Bold,
		Red,
		Purple,
		Blue,
		Gray
	};

	class CharArray;

	class Stream
	{
		public:
			void puts(const CharArray &string);
		
			virtual void color(Color color, const CharArray &string);
			virtual void print(const CharArray &string) =  0;

			virtual ~Stream() {}
	};

	class CharArrayStream:
		public Stream
	{
		private:
			CharArray &str;
			OnStackString<1> os;

		public:
			CharArrayStream(CharArray &str);
			void print(const CharArray &string);
	};
};
