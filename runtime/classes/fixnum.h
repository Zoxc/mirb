#pragma once
#include "../../globals.h"
#include "../classes.h"

#define RT_INT2FIX(imm) ((rt_value)(((rt_value)(imm) << 1) | RT_FLAG_FIXNUM))
#define RT_FIX2INT(imm) ((rt_value)((rt_value)(imm) >> 1))


extern rt_value rt_Fixnum;

void rt_fixnum_init();

rt_value __stdcall rt_fixnum_to_s(rt_value obj, rt_value block, size_t argc, rt_value argv[]);
