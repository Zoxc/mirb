#pragma once
#include "runtime.h"

enum rt_exception_type {
	E_RUBY_EXCEPTION,
	E_RETURN_EXCEPTION,
	E_BREAK_EXCEPTION,
	E_THROW_EXCEPTION
};

struct rt_upval {
	union {
		rt_value *upval;
		rt_value local;
	} val;
	bool sealed;
};

void __stdcall rt_support_seal_upval(struct rt_upval *upval);

rt_value __cdecl rt_support_interpolate(size_t argc, rt_value argv[]);

rt_value __cdecl rt_support_array(size_t argc, rt_value argv[]);

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name);
void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value);

rt_value __stdcall rt_support_define_string(const char* string);

