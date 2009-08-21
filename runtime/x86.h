#pragma once
#include "../globals.h"
#include "classes.h"
#include "support.h"

rt_value rt_support_closure(rt_compiled_closure_t block, unsigned int argc, ...);

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super);
void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block);

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj);
rt_upval_t *rt_support_upval_create();

rt_value __stdcall rt_support_get_upval(rt_upval_t **upvals);
void __stdcall rt_support_set_upval(rt_upval_t **upvals, rt_value value);
