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
	static bool rt_find_seh_target(void *target)
	{
		rt_seh_frame_t *frame;

		__asm__ __volatile__("movl %%fs:(0), %0\n" : "=r" (frame));

		while(true)
		{
			if(frame == (void *)-1)
				return false;

			if(frame->handler == rt_support_seh_handler)
			{
				if(frame->block == target)
					return true;
			}

			frame = frame->prev;
		}
	}

	void __stdcall __attribute__((noreturn)) rt_support_return(rt_value value, void *target)
	{
		if(!rt_find_seh_target(target))
			assert(0);

		rt_value data[2];

		data[0] = (rt_value)target;
		data[1] = value;

		RaiseException(RT_SEH_RUBY + E_RETURN_EXCEPTION, 0, 2, (const DWORD *)&data);

		__builtin_unreachable();
	}

	void __stdcall __attribute__((noreturn)) rt_support_break(rt_value value, void *target, size_t id)
	{
		if(!rt_find_seh_target(target))
			assert(0);

		rt_value data[3];

		data[0] = (rt_value)target;
		data[1] = value;
		data[2] = id;

		RaiseException(RT_SEH_RUBY + E_BREAK_EXCEPTION, 0, 3, (const DWORD *)&data);

		__builtin_unreachable();
	}

	extern void __stdcall RtlUnwind(
		PVOID TargetFrame,
		PVOID TargetIp,
		PEXCEPTION_RECORD ExceptionRecord,
		PVOID ReturnValue
	);

	static void rt_seh_global_unwind(rt_seh_frame_t *target)
	{
		int dummy;

		__asm__ __volatile__("pushl %%ebp\n"
			"pushl %%ebx\n"
			"pushl $0\n"
			"pushl $0\n"
			"pushl 0f\n"
			"pushl %0\n"
			"call _RtlUnwind@16\n"
			"0:\n"
			"popl %%ebx\n"
			"popl %%ebp\n"
		: "=a" (dummy)
		: "0" (target)
		: "esi", "edi", "edx", "ecx", "memory");
	}

	static void rt_seh_local_unwind(rt_seh_frame_t *frame_data, exception_block_t *block, exception_block_t *target)
	{
		frame_data->handling = 1;

		size_t ebp = (size_t)&frame_data->old_ebp;

		while(block != target)
		{
			if(block->ensure_label)
			{
				#ifdef DEBUG
					printf("Ensure block found\n");
					printf("Ebp: %x\n", ebp);
					printf("Eip: %x\n", block->ensure_label);
				#endif

				int dummy1, dummy2;

				__asm__ __volatile__("pushl %%ebp\n"
					"pushl %%ebx\n"
					"mov %0, %%ebp\n"
					"call *%1\n"
					"popl %%ebx\n"
					"popl %%ebp\n"
				: "=a" (dummy1), "=c" (dummy2)
				: "0" (ebp), "1" (block->ensure_label)
				: "esi", "edi", "edx", "memory");
			}

			block = block->parent;
		}

		frame_data->handling = 0;
	}

	static void __attribute__((noreturn)) rt_seh_return(rt_seh_frame_t *frame_data, exception_block_t *block, rt_value value)
	{
		rt_seh_global_unwind(frame_data);

		/*
		 * Execute frame local ensure handlers
		 */

		rt_seh_local_unwind(frame_data, block, 0);

		/*
		 * Go to the handler and never return
		 */

		size_t ebp = (size_t)&frame_data->old_ebp;

		#ifdef DEBUG
			printf("Return target found\n");
			printf("Ebp: %x\n", ebp);
			printf("Esp: %x\n", ebp - 20 - 12 - frame_data->block->local_storage);
			printf("Eip: %x\n", frame_data->block->epilog);
		#endif

		__asm__ __volatile__("mov %0, %%ecx\n"
			"mov %1, %%esp\n"
			"mov %2, %%ebp\n"
			"jmp *%%ecx\n"
		:
		: "r" (frame_data->block->epilog),
			"r" (ebp - 20 - 12 - frame_data->block->local_storage),
			"r" (ebp),
			"a" (value)
		: "%ecx");

		__builtin_unreachable();
	}

	static void __attribute__((noreturn)) rt_seh_break(rt_seh_frame_t *frame_data, rt_value value, size_t id)
	{
		rt_seh_global_unwind(frame_data);

		/*
		 * Go to the handler and never return
		 */

		size_t ebp = (size_t)&frame_data->old_ebp;

		#ifdef DEBUG
			printf("Break target found\n");
			printf("Ebp: %x\n", ebp);
			printf("Esp: %x\n", ebp - 20 - 12 - frame_data->block->local_storage);
			printf("Eip: %x\n", frame_data->block->break_targets[id]);
		#endif

		__asm__ __volatile__("mov %0, %%ecx\n"
			"mov %1, %%esp\n"
			"mov %2, %%ebp\n"
			"jmp *%%ecx\n"
		:
		: "r" (frame_data->block->break_targets[id]),
			"r" (ebp - 20 - 12 - frame_data->block->local_storage),
			"r" (ebp),
			"a" (value)
		: "%ecx");

		__builtin_unreachable();
	}

	static void __attribute__((noreturn)) rt_seh_rescue(rt_seh_frame_t *frame_data, exception_block_t *block, exception_block_t *current_block, void *rescue_label)
	{
		rt_seh_global_unwind(frame_data);

		/*
		 * Execute frame local ensure handlers
		 */

		rt_seh_local_unwind(frame_data, block, current_block);

		/*
		 * Set to the current handler index to the parent
		 */

		frame_data->block_index = current_block->parent_index;

		/*
		 * Go to the handler and never return
		 */

		size_t ebp = (size_t)&frame_data->old_ebp;

		#ifdef DEBUG
			printf("Rescue block found\n");
			printf("Ebp: %x\n", ebp);
			printf("Esp: %x\n", ebp - 20 - 12 - frame_data->block->local_storage);
			printf("Eip: %x\n", rescue_label);
		#endif

		__asm__ __volatile__("mov %0, %%eax\n"
			"mov %1, %%esp\n"
			"mov %2, %%ebp\n"
			"jmp *%%eax\n"
		:
		: "r" (rescue_label),
			"r" (ebp - 20 - 12 - frame_data->block->local_storage),
			"r" (ebp)
		: "%eax");

		__builtin_unreachable();
	}

	EXCEPTION_DISPOSITION __cdecl rt_support_seh_handler(EXCEPTION_RECORD *exception, rt_seh_frame_t *frame_data, CONTEXT *context, void *dispatcher_context)
	{
		if(frame_data->block_index == -1)
		{
			if(exception->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
				return ExceptionContinueSearch;
			else if(exception->ExceptionCode == RT_SEH_RUBY + E_RETURN_EXCEPTION && (void *)exception->ExceptionInformation[0] == frame_data->block)
			{
				rt_seh_return(frame_data, 0, exception->ExceptionInformation[1]);
			}
			else if(exception->ExceptionCode == RT_SEH_RUBY + E_BREAK_EXCEPTION && (void *)exception->ExceptionInformation[0] == frame_data->block)
			{
				rt_seh_break(frame_data, exception->ExceptionInformation[1], exception->ExceptionInformation[2]);
			}
		}

		exception_block_t *block = kv_A(frame_data->block->exception_blocks, frame_data->block_index);

		if(exception->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
		{
			rt_seh_local_unwind(frame_data, block, 0);

			return ExceptionContinueSearch;
		}

		switch(exception->ExceptionCode)
		{
			case RT_SEH_RUBY + E_BREAK_EXCEPTION:
			{
				rt_seh_break(frame_data, exception->ExceptionInformation[1], exception->ExceptionInformation[2]);
			}
			break;

			case RT_SEH_RUBY + E_RETURN_EXCEPTION:
			{
				rt_seh_return(frame_data, block, exception->ExceptionInformation[1]);
			}
			break;

			case RT_SEH_RUBY + E_RUBY_EXCEPTION:
			{
				exception_block_t *current_block = block;

				while(current_block)
				{
					for(size_t i = 0; i < kv_size(current_block->handlers); i++)
					{
						struct exception_handler *handler = kv_A(current_block->handlers, i);

						switch(handler->type)
						{
							case E_RUNTIME_EXCEPTION:
								rt_seh_rescue(frame_data, block, current_block, ((struct runtime_exception_handler *)handler)->rescue_label);

							default:
								break;
						}
					}

					current_block = current_block->parent;
				}
			}
			break;
		}

		return ExceptionContinueSearch;
	}
#endif
