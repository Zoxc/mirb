#pragma once
#include "../globals.h"
#include "classes.h"
#include "support.h"

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj);
rt_upval_t *rt_support_upval_create();

rt_value __stdcall rt_support_get_upval(rt_value closure);
void __stdcall rt_support_set_upval(rt_value closure, rt_value value);
