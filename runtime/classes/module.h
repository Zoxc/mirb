#pragma once
#include "../../globals.h"
#include "../runtime.h"
#include "object.h"
#include "class.h"

extern rt_value rt_Module;

rt_value __cdecl rt_module_include(rt_value obj, size_t argc, ...);

void rt_module_init(void);
