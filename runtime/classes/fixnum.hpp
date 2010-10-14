#pragma once
#include "../../globals.hpp"
#include "../classes.hpp"
#include "string.hpp"

#define RT_INT2FIX(imm) ((rt_value)(((rt_value)(imm) << 1) | RT_FLAG_FIXNUM))
#define RT_FIX2INT(imm) ((rt_value)((rt_value)(imm) >> 1))

extern rt_value rt_Fixnum;

void rt_fixnum_init();

extern rt_value rt_string_from_cstr(const char *str);

static inline rt_value rt_string_from_fixnum(rt_value value)
{
	char buffer[15];

	sprintf(buffer, "%d", RT_FIX2INT(value));

	return rt_string_from_cstr(buffer);
}

rt_compiled_block(rt_fixnum_to_s);
