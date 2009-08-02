#pragma once
#include "runtime.h"

void __stdcall rt_method_create_main(rt_value name, rt_compiled_block_t block);
void __stdcall rt_method_create(rt_value under, rt_value name, rt_compiled_block_t block);
