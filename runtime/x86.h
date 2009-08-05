#pragma once
#include "../globals.h"
#include "classes.h"

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name);
void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value);

rt_value __stdcall rt_support_define_class(rt_value under, rt_value name, rt_value super);

rt_value __stdcall rt_support_define_string(const char* string);

void __stdcall rt_support_define_method(rt_value under, rt_value name, rt_compiled_block_t block);
rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value method, rt_value obj);
