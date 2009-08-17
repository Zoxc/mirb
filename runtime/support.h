#pragma once
#include "runtime.h"

typedef struct {
	union {
		rt_value *upval;
		rt_value local;
	} val;
	bool sealed;
} rt_upval_t;

void __stdcall rt_support_seal_upval(rt_upval_t *upval);

rt_value rt_support_interpolate(unsigned int argc, ...);

rt_value rt_support_closure(rt_compiled_block_t block, unsigned int argc, ...);

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name);
void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value);

rt_value __stdcall rt_support_define_class(rt_value under, rt_value name, rt_value super);

rt_value __stdcall rt_support_define_string(const char* string);

void __stdcall rt_support_define_method(rt_value under, rt_value name, rt_compiled_block_t block);
