#pragma once
#include "../classes.hpp"
#include "../support.hpp"

#include "../../src/block.hpp"

struct rt_proc {
	struct rt_common common;
	rt_value self;
	Mirb::Block *closure;
	size_t scope_count;
	rt_value *scopes[];
};

extern rt_value rt_Proc;

#define RT_PROC(value) ((struct rt_proc *)(value))

static inline rt_value rt_call_proc(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return rt_call_block(obj, rt_symbol_from_cstr("call"), block, argc, argv);
}

void rt_proc_init(void);

rt_compiled_block(rt_proc_call);
