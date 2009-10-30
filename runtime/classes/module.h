#pragma once
#include "../../globals.h"
#include "../runtime.h"
#include "object.h"
#include "class.h"

extern rt_value rt_Module;

rt_compiled_block(rt_module_include);

void rt_module_init(void);
