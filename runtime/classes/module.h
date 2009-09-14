#pragma once
#include "../../globals.h"
#include "../runtime.h"
#include "object.h"
#include "class.h"

extern rt_value rt_Module;

rt_value __stdcall rt_module_include(rt_value obj, rt_value block, size_t argc, rt_value argv[]);

void rt_module_init(void);
