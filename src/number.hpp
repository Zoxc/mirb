#pragma once
#include "char-array.hpp"
#include <tommath.h>

namespace Mirb
{
	class Number
	{
		private:
			mp_int num;
			
			intptr_t to_big_endian(intptr_t in);
			void check(int error);

		public:
			Number();
			Number(Number &&other);
			Number(const Number &other);
			Number(size_t input);
			Number(intptr_t input);
			Number(const CharArray &string, size_t base = 10);
			~Number();
	};
};
