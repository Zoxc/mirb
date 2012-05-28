#include "number.hpp"
#include "runtime.hpp"
#include "classes/fixnum.hpp"
#include "classes/bignum.hpp"

#ifdef _MSC_VER
	#include <intrin.h>
#endif

namespace Mirb
{
	mp_int Number::fixnum_high;
	mp_int Number::fixnum_low;

	void Number::initialize()
	{
		Number high(Fixnum::high);
		Number low(Fixnum::low);
		
		check(mp_init_copy(&fixnum_high, &high.num));
		check(mp_init_copy(&fixnum_low, &low.num));
	}
	
	void Number::finalize()
	{
		mp_clear(&fixnum_high);
		mp_clear(&fixnum_low);
	}

	void Number::check(int error)
	{
		if(error != MP_OKAY)
			raise(context->runtime_error, "Unable to perform Bignum operation: " + CharArray((const char_t *)mp_error_to_string(error)));
	}
	
	int Number::compare(const Number &other) const
	{
		int result = mp_cmp(const_cast<mp_int *>(&num), const_cast<mp_int *>(&other.num));

		switch(result)
		{
			case MP_GT:
				return 1;

			case MP_EQ:
				return 0;

			case MP_LT:
				return -1;

			default:
				check(result);
				mirb_debug_abort("Unknown result");
		}
	}
			
	bool Number::can_be_fix() const
	{
		if(mp_cmp(const_cast<mp_int *>(&num), &fixnum_high) == MP_GT)
			return false;

		if(mp_cmp(const_cast<mp_int *>(&num), &fixnum_low) == MP_LT)
			return false;

		return true;
	}

	intptr_t Number::swap_endian(intptr_t in)
	{
#ifdef _MSC_VER
		if(sizeof(intptr_t) == 8)
			return (intptr_t)_byteswap_uint64(in);
		else
			return (intptr_t)_byteswap_ulong(in);
#else
		if(sizeof(intptr_t) == 8)
			return (intptr_t)__builtin_bswap64(in);
		else
			return (intptr_t)__builtin_bswap32(in);
#endif
	}

	Number::~Number()
	{
		mp_clear(&num);
	}

	Number::Number()
	{
		check(mp_init(&num));
	}
	
	Number::Number(Number &&other) : num(other.num)
	{
		other.num.dp = 0;
	}

	Number::Number(const Number &other)
	{
		check(mp_init_copy(&num, const_cast<mp_int *>(&other.num)));
	}
	
	Number::Number(size_t input)
	{
		check(mp_init(&num));
		
		if(sizeof(intptr_t) == sizeof(unsigned long))
		{
			mp_set_int(&num, (unsigned long)input);
			return;
		}

		intptr_t big = swap_endian((intptr_t)input);

		mp_read_unsigned_bin(&num, (const unsigned char *)&big, sizeof(big));
	}

	Number::Number(intptr_t input)
	{
		check(mp_init(&num));

		if(sizeof(intptr_t) == sizeof(unsigned long))
		{
			if(input < 0)
			{
				input = -input;
				mp_set_int(&num, (unsigned long)input);
				check(mp_neg(&num, &num));
			}
			else
				mp_set_int(&num, (unsigned long)input);

			return;
		}

		if(input < 0)
		{
			input = -input;
			input = swap_endian(input);
			check(mp_read_unsigned_bin(&num, (const unsigned char *)&input, sizeof(intptr_t)));
			check(mp_neg(&num, &num));
		}
		else
		{
			input = swap_endian(input);
			mp_read_unsigned_bin(&num, (const unsigned char *)&input, sizeof(intptr_t));
		}
	}

	Number::Number(const CharArray &string, size_t base)
	{
		check(mp_init(&num));

		CharArray null_str = string.c_str();

		check(mp_read_radix(&num, null_str.c_str_ref(), base));
	}

	Number::Number(const void *storage, size_t size)
	{
		check(mp_init(&num));

		mp_read_signed_bin(&num, (const unsigned char *)storage, size);
	}
	
	intptr_t Number::to_intptr() const
	{
		if(sizeof(intptr_t) == sizeof(unsigned long))
			return (intptr_t)mp_get_int(const_cast<mp_int *>(&num));

		unsigned long size = sizeof(intptr_t);

		intptr_t result = 0;

		int left = sizeof(intptr_t) - mp_unsigned_bin_size(const_cast<mp_int *>(&num));

		mirb_debug_assert(left >= 0);

		mp_to_unsigned_bin_n(const_cast<mp_int *>(&num), (unsigned char *)&result + left, &size);

		result = swap_endian(result);

		if(SIGN(&num))
			result = -result;

		return result;
	}
	
	Bignum *Number::to_bignum() const
	{
		return new (collector) Bignum(*this);
	}

	value_t Number::to_value() const
	{
		if(can_be_fix())
			return Fixnum::from_int(to_intptr());
		else
			return to_bignum();
	}

	CharArray Number::to_string(size_t base) const
	{
		CharArray result;

		int size;

		check(mp_radix_size(const_cast<mp_int *>(&num), base, &size));

		result.buffer(size);
		
		check(mp_toradix_n(const_cast<mp_int *>(&num), (char *)result.str_ref(), base, size));

		result.shrink(result.size() - 1);

		return result;
	}

	Number Number::operator *(const Number &other) const
	{
		return op(other, [&](mp_int *self, mp_int *other, mp_int *result) { return mp_mul(self, other, result); });
	}

	Number Number::operator +(const Number &other) const
	{
		return op(other, [&](mp_int *self, mp_int *other, mp_int *result) { return mp_add(self, other, result); });
	}

	Number Number::operator -(const Number &other) const
	{
		return op(other, [&](mp_int *self, mp_int *other, mp_int *result) { return mp_sub(self, other, result); });
	}

	Number Number::operator /(const Number &other) const
	{
		return op(other, [&](mp_int *self, mp_int *other, mp_int *result) { return mp_div(self, other, result, 0); });
	}

	Number Number::mod(const Number &other) const
	{
		return op(other, [&](mp_int *self, mp_int *other, mp_int *result) { return mp_mod(self, other, result); });
	}

	Number Number::neg() const
	{
		Number result;

		check(mp_neg(const_cast<mp_int *>(&num), &result.num));

		return result;
	}
};
