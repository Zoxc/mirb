#pragma once
#include "../classes.h"
#include "../support.h"

struct rt_proc {
	struct rt_common common;
	rt_value self;
	rt_compiled_block_t closure;
	size_t scope_count;
	rt_value *scopes[];
};

extern rt_value rt_Proc;

#define RT_PROC(value) ((struct rt_proc *)(value))

void rt_proc_init(void);

rt_compiled_block(rt_proc_call);
