#pragma once
#include "char-array.hpp"
#include <tommath.h>

namespace Mirb
{
	class Bignum;

	class Number
	{
		private:
			mp_int num;

			static mp_int fixnum_high;
			static mp_int fixnum_low;
			
			static intptr_t swap_endian(intptr_t in);
			static void check(int error);

			template<typename F> Number op(const Number &other, F func) const
			{
				Number result;

				check(func(const_cast<mp_int *>(&num), const_cast<mp_int *>(&other.num), &result.num));

				return result;
			};

		public:
			static Number *zero;

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
			
			int compare(const Number &other) const;
			
			template<typename A> void to_binary(void *&storage, size_t &size, typename A::Reference ref = A::default_reference) const
			{
				size = mp_signed_bin_size((mp_int *)&num);
				storage = A(ref).allocate(size);

				check(mp_to_signed_bin((mp_int *)&num, (unsigned char *)storage));
			}
			
			bool operator ==(const Number &other) const
			{
				return compare(other) == 0;
			}
			
			bool can_be_fix() const;
		
			intptr_t to_intptr() const;
			value_t to_value() const;
			Bignum *to_bignum() const;

			CharArray to_string(size_t base = 10) const;

			Number neg() const;

			Number operator *(const Number &other) const;
			Number operator /(const Number &other) const;
			Number operator +(const Number &other) const;
			Number operator -(const Number &other) const;
			Number mod(const Number &other) const;
	};
};
