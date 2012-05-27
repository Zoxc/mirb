#include "number.hpp"
#include "runtime.hpp"

#ifdef _MSC_VER
	#include <intrin.h>
#endif

namespace Mirb
{
	void Number::check(int error)
	{
		if(error != MP_OKAY)
			raise(context->runtime_error, "Unable to perform Bignum operation: " + CharArray((const char_t *)mp_error_to_string(error)));
	}

	intptr_t Number::to_big_endian(intptr_t in)
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
	}

	Number::Number(const Number &other)
	{
		check(mp_init_copy(&num, (mp_int *)&other.num));
	}
	
	Number::Number(size_t input)
	{
		check(mp_init(&num));

		if(sizeof(intptr_t) == 8)
		{
			intptr_t big = to_big_endian((intptr_t)input);

			mp_read_unsigned_bin(&num, (const unsigned char *)&big, sizeof(big));
		}
		else
			mp_set_int(&num, input);
	}

	Number::Number(intptr_t input)
	{
		check(mp_init(&num));

		if(sizeof(intptr_t) == 8)
		{
			intptr_t big = to_big_endian(input);

			if(big < 0)
			{
				big = -big;
				check(mp_read_unsigned_bin(&num, (const unsigned char *)&big, sizeof(big)));
				check(mp_neg(&num, &num));
			}
			else
				mp_read_unsigned_bin(&num, (const unsigned char *)&big, sizeof(big));
		}
		else
			mp_set(&num, (unsigned int)input);
	}

	Number::Number(const CharArray &string, size_t base)
	{
		check(mp_init(&num));
	}
};
