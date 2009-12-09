#include "../classes.h"
#include "../runtime.h"
#include "proc.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Proc;

rt_compiled_block(rt_proc_call)
{
	return RT_PROC(obj)->closure((rt_value **)&RT_PROC(obj)->scopes, RT_PROC(obj)->method_name, RT_PROC(obj)->method_module, RT_PROC(obj)->self, block, argc, argv);
}

void rt_proc_init(void)
{
	rt_Proc = rt_define_class(rt_Object, rt_symbol_from_cstr("Proc"), rt_Object);
	rt_define_method(rt_Proc, rt_symbol_from_cstr("call"), (rt_compiled_block_t)rt_proc_call);
}
