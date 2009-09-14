#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "../classes/symbol.h"
#include "../classes/string.h"

rt_value rt_Kernel;

rt_value __stdcall rt_kernel_proc(rt_value obj, rt_value block, size_t argc, rt_value argv[])
{
	if(block)
		return block;
	else
		return RT_NIL;
}

void rt_kernel_init(void)
{
	rt_Kernel = rt_define_module(rt_Object, rt_symbol_from_cstr("Kernel"));

	rt_include_module(rt_Object, rt_Kernel);

    rt_define_method(rt_Kernel, rt_symbol_from_cstr("proc"), rt_kernel_proc);
}
