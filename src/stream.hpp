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
			enum PosType
			{
				FromStart,
				FromCurrent,
				FromEnd
			};

			typedef int64_t pos_t ;

			virtual void puts(const CharArray &string);
			virtual CharArray read();
			virtual CharArray read(size_t length);
			virtual void seek(pos_t val, PosType type = FromCurrent);
			virtual pos_t pos();
			virtual pos_t size();
			virtual CharArray gets();
			virtual void color(Color color, const CharArray &string);
			virtual void print(const CharArray &string) = 0;

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
