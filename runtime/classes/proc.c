#include "../classes.h"
#include "../runtime.h"
#include "proc.h"
#include "symbol.h"
#include "string.h"

rt_value rt_Proc;

rt_value rt_proc_call(rt_value obj, size_t argc, ...)
{
	printf("calling block %x\n", obj);

	rt_value args[argc];

	va_list _args;
	va_start(_args, argc);

	for(int i = argc - 1; i >= 0; i--)
		args[i] = va_arg(_args, rt_value);

	va_end(_args);

	switch(argc)
	{
		case 0:
			return RT_PROC(obj)->closure(RT_PROC(obj)->upvals, RT_PROC(obj)->self, argc);

		case 1:
			return RT_PROC(obj)->closure(RT_PROC(obj)->upvals, RT_PROC(obj)->self, argc, args[0]);

		case 2:
			return RT_PROC(obj)->closure(RT_PROC(obj)->upvals, RT_PROC(obj)->self, argc, args[0], args[1]);

		case 3:
			return RT_PROC(obj)->closure(RT_PROC(obj)->upvals, RT_PROC(obj)->self, argc, args[0], args[1], args[2]);

		case 4:
			return RT_PROC(obj)->closure(RT_PROC(obj)->upvals, RT_PROC(obj)->self, argc, args[0], args[1], args[2], args[3]);

		default:
			assert(0);
	}

	return RT_NIL;
}

void rt_proc_init(void)
{
	rt_Proc = rt_define_class(rt_Object, rt_symbol_from_cstr("Proc"), rt_Object);
	rt_define_method(rt_Proc, rt_symbol_from_cstr("call"), (rt_compiled_block_t)rt_proc_call);
}
