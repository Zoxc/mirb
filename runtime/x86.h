#pragma once
#include "../globals.h"
#include "classes.h"

rt_value __stdcall rt_support_get_const(rt_value obj, rt_value name);
void __stdcall rt_support_set_const(rt_value obj, rt_value name, rt_value value);

rt_value __stdcall rt_support_class_create_main(rt_value name, rt_value super);
rt_value __stdcall rt_support_class_create(rt_value under, rt_value name, rt_value super);
