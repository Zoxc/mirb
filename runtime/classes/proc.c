#include "../classes.h"
#include "../runtime.h"
#include "proc.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Proc;

rt_compiled_block(rt_proc_call)
{
	rt_value result;

	__asm__("push %[argv]\n"
		"push %[argc]\n"
		"push %[block]\n"
		"push %[obj]\n"
		"call *%[proc]\n"
		: "=a" (result)
		: [proc] "g" (RT_PROC(obj)->closure),
			[obj] "g" (RT_PROC(obj)->self),
			[block] "g" (block),
			[argc] "g" (argc),
			[argv] "g" (argv),
			"a" (&RT_PROC(obj)->scopes)
		: "%ecx", "%edx");

	return result;
}

void rt_proc_init(void)
{
	rt_Proc = rt_define_class(rt_Object, rt_symbol_from_cstr("Proc"), rt_Object);
	rt_define_method(rt_Proc, rt_symbol_from_cstr("call"), (rt_compiled_block_t)rt_proc_call);
}
