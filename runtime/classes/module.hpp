#pragma once
#include "../../globals.hpp"
#include "../runtime.hpp"
#include "object.hpp"
#include "class.hpp"

extern rt_value rt_Module;

rt_compiled_block(rt_module_include);

void rt_module_init(void);
