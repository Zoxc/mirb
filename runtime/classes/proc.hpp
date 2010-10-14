#pragma once
#include "../classes.hpp"
#include "../support.hpp"

struct rt_proc {
	struct rt_common common;
	rt_value self;
	rt_value method_name;
	rt_value method_module;
	struct rt_block *closure;
	size_t scope_count;
	rt_value *scopes[];
};

extern rt_value rt_Proc;

#define RT_PROC(value) ((struct rt_proc *)(value))

static inline rt_value rt_call_proc(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	return RT_PROC(obj)->closure->compiled((rt_value **)&RT_PROC(obj)->scopes, RT_PROC(obj)->method_name, RT_PROC(obj)->method_module, RT_PROC(obj)->self, block, argc, argv);
}

void rt_proc_init(void);

rt_compiled_block(rt_proc_call);
