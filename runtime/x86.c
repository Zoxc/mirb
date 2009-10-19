#include "classes.h"
#include "runtime.h"
#include "constant.h"
#include "x86.h"
#include "classes/symbol.h"
#include "classes/string.h"
#include "classes/proc.h"

rt_value __cdecl rt_support_closure(rt_compiled_block_t block, size_t argc, rt_upval_t *argv[])
{
	rt_value self;

	__asm__("" : "=D" (self));

	rt_value closure = (rt_value)malloc(sizeof(struct rt_proc));

	RT_COMMON(closure)->flags = C_PROC;
	RT_COMMON(closure)->class_of = rt_Proc;
	RT_PROC(closure)->self = self;
	RT_PROC(closure)->closure = block;
	RT_PROC(closure)->upval_count = argc;
	RT_PROC(closure)->upvals = malloc(sizeof(rt_upval_t *) * argc);

	RT_ARG_EACH_RAW(i)
	{
		RT_PROC(closure)->upvals[i] = RT_ARG(i);
	}

	return closure;
}

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj)
{
	rt_value method;

	__asm__("" : "=a" (method));
/*
	rt_compiled_block_t result = rt_lookup(obj, method);

	__asm__("jmp %0" : : "r" (result));
*/
	return rt_lookup(obj, method);
}

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_class(obj, name, super);
}

rt_value __stdcall rt_support_define_module(rt_value name)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_module(obj, name);
}

void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block)
{
	rt_value obj;

	__asm__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	rt_define_method(obj, name, block);
}


rt_upval_t *rt_support_upval_create(void)
{
	rt_value *real;

	__asm__("" : "=a" (real));

	rt_upval_t *result = malloc(sizeof(rt_upval_t));

	result->val.upval = real;
	result->sealed = false;

	return result;
}

rt_value rt_support_get_ivar(void)
{
	rt_value obj;
	rt_value name;

	__asm__("" : "=D" (obj), "=a" (name));

	return rt_object_get_var(obj, name);
}

void __stdcall rt_support_set_ivar(rt_value value)
{
	rt_value obj;
	rt_value name;

	__asm__("" : "=D" (obj), "=a" (name));

	rt_object_set_var(obj, name, value);
}

rt_value rt_support_get_upval(void)
{
	size_t index;
	rt_upval_t **upvals;

	__asm__("" : "=a" (index), "=S" (upvals));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		return *(upval->val.upval);
	else
		return upval->val.local;
}

void __stdcall rt_support_set_upval(rt_value value)
{
	size_t index;
	rt_upval_t **upvals;

	__asm__("" : "=a" (index), "=S" (upvals));

	rt_upval_t *upval = upvals[index];

	if(!upval->sealed)
		*(upval->val.upval) = value;
	else
		upval->val.local = value;
}

#ifdef WINDOWS
	extern void WINAPI RtlUnwind(
	  PVOID TargetFrame,
	  PVOID TargetIp,
	  PEXCEPTION_RECORD ExceptionRecord,
	  PVOID ReturnValue
	);

	EXCEPTION_DISPOSITION __cdecl rt_seh_handler(EXCEPTION_RECORD *exception, rt_seh_frame_t *frame_data, CONTEXT *context, void *dispatcher_context)
	{
		if(exception->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
		{
			return ExceptionContinueSearch;
		}
		else
		{
			if(exception->ExceptionCode == RT_SEH_RUBY_EXCEPTION)
			{

				exception_handler_t *handler = &frame_data->block->handlers[frame_data->handler_index];
				exception_handler_t *current = handler;

				while(current)
				{
					if(current->rescue)
					{
						RtlUnwind(frame_data, &&continue_label, 0, 0);

						continue_label:
						MessageBox(0, "hi", 0, 0);

						if(handler->parent)
							frame_data->handler_index = (current - frame_data->block->handlers) / sizeof(exception_handler_t);
						else
							frame_data->handler_index = -1;

						printf("Rescue block found\n");
						printf("Ebp: %x\n", frame_data->ebp);
						printf("Esp: %x\n", frame_data->ebp - 20 - frame_data->block->local_storage);
						printf("Eip: %x\n", current->rescue);
						printf("New handler: %d\n", frame_data->handler_index);

						__asm__("mov %0, %%eax\n"
							"mov %1, %%esp\n"
							"mov %2, %%ebp\n"
							"jmp *%%eax\n"
						:
						: "r" (current->rescue),
							"r" (frame_data->ebp - 20 - frame_data->block->local_storage),
							"r" (frame_data->ebp)
						: "%eax");
					}

					current = current->parent;
				}
			}

			return ExceptionContinueSearch;
		}
	}
#endif
