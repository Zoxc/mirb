#include "fixnum.hpp"
#include "object.hpp"
#include "../char-array.hpp"
#include "../runtime.hpp"

namespace Mirb
{
	value_t Fixnum::class_ref;
	
	static value_t int_to_fix(size_t imm)
	{
		return (imm << 1) | 1;
	}
	
	static size_t fix_to_int(value_t obj)
	{
		return obj >> 1;
	}
	
	value_t Fixnum::from_size_t(size_t value)
	{
		return int_to_fix(value);
	}

	size_t Fixnum::to_size_t(value_t obj)
	{
		return fix_to_int(obj);
	}
	
	value_t Fixnum::from_int(int value)
	{
		return int_to_fix((size_t)value);
	}

	int Fixnum::to_int(value_t obj)
	{
		return (int)fix_to_int(obj);
	}
	
	mirb_compiled_block(fixnum_to_s)
	{
		char_t buffer[15];

		size_t length = sprintf((char *)buffer, "%d", fix_to_int(obj));

		return CharArray(buffer, length).to_string();
	}

	mirb_compiled_block(fixnum_add)
	{
		return int_to_fix(fix_to_int(obj) + fix_to_int(MIRB_ARG(0)));
	}

	mirb_compiled_block(fixnum_sub)
	{
		return int_to_fix(fix_to_int(obj) - fix_to_int(MIRB_ARG(0)));
	}

	mirb_compiled_block(fixnum_mul)
	{
		return int_to_fix(fix_to_int(obj) * fix_to_int(MIRB_ARG(0)));
	}

	mirb_compiled_block(fixnum_div)
	{
		return int_to_fix(fix_to_int(obj) / fix_to_int(MIRB_ARG(0)));
	}

	void Fixnum::initialize()
	{
		Fixnum::class_ref = define_class(Object::class_ref, "Fixnum", Object::class_ref);

		define_method(Fixnum::class_ref, "to_s", fixnum_to_s);

		define_method(Fixnum::class_ref, "+", fixnum_add);
		define_method(Fixnum::class_ref, "-", fixnum_sub);
		define_method(Fixnum::class_ref, "*", fixnum_mul);
		define_method(Fixnum::class_ref, "/", fixnum_div);
	}
};

