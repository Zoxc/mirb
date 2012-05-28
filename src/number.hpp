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

			template<typename F> Number op(const Number &other, F func)
			{
				Number result;

				check(func(const_cast<mp_int *>(&other.num), &result.num));

				return result;
			};

		public:
			Number();
			Number(Number &&other);
			Number(const Number &other);
			Number(size_t input);
			Number(intptr_t input);
			Number(const CharArray &string, size_t base = 10);
			~Number();

			value_t to_value();

			CharArray to_string(size_t base = 10);

			Number operator *(Number other);
	};

	//Number operator *(Number lhs, Number rhs);
};
