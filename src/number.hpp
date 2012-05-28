#pragma once
#include "char-array.hpp"
#include <tommath.h>

namespace Mirb
{
	class Number
	{
		private:
			mp_int num;

			static mp_int fixnum_high;
			static mp_int fixnum_low;
			
			intptr_t swap_endian(intptr_t in);
			static void check(int error);

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
			Number(const void *storage, size_t size);
			~Number();

			static void initialize();
			static void finalize();
			
			template<typename A> void to_binary(void *&storage, size_t &size, typename A::Reference ref = A::default_reference) const
			{
				size = mp_signed_bin_size((mp_int *)&num);
				storage = A(ref).allocate(size);

				check(mp_to_signed_bin((mp_int *)&num, (unsigned char *)storage));
			}

			bool can_be_fix();
		
			intptr_t to_intptr();
			value_t to_value();

			CharArray to_string(size_t base = 10);

			Number operator *(Number other);
	};

	//Number operator *(Number lhs, Number rhs);
};
