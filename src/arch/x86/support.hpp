#pragma once
#include "../../common.hpp"
#include "../../../runtime/classes.hpp"

rt_value __fastcall rt_support_call(rt_value method_name, rt_value dummy, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
rt_value __fastcall rt_support_super(rt_value method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
