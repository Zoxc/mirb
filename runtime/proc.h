#pragma once
#include "classes.h"
#include "support.h"

struct rt_proc {
	struct rt_common common;
	rt_compiled_block_t block;
	rt_upval_t **upvals;
	unsigned int upval_count;
};

extern rt_value rt_Proc;

#define RT_PROC(value) ((struct rt_proc *)value)

void rt_proc_init(void);

rt_value __cdecl rt_proc_call(rt_value obj, unsigned int argc, ...);
