#include "../classes.h"
#include "../runtime.h"
#include "../constant.h"
#include "../classes/symbol.h"
#include "../classes/string.h"

rt_value rt_Kernel;

rt_value __cdecl rt_kernel_proc(rt_value obj, size_t argc)
{
	rt_value block;

	__asm__("" : "=c" (block));

	if(block)
		return block;
	else
		return RT_NIL;
}

void rt_kernel_init(void)
{
	rt_Kernel = rt_define_module(rt_Object, rt_symbol_from_cstr("Kernel"));

	rt_include_module(rt_Object, rt_Kernel);

	printf("rt_Kernel is %x\n", rt_Kernel);

    rt_define_method(rt_Kernel, rt_symbol_from_cstr("proc"), (rt_compiled_block_t)rt_kernel_proc);
}
