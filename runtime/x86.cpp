#include "classes.hpp"
#include "runtime.hpp"
#include "constant.hpp"
#include "x86.hpp"
#include "exceptions.hpp"
#include "classes/symbol.hpp"
#include "classes/string.hpp"
#include "classes/proc.hpp"

#ifdef DEBUG
	rt_value __stdcall __regparm(2) rt_support_call(rt_value dummy, rt_value method_name, rt_value obj, rt_value block, size_t argc, rt_value argv[])
	{
		rt_value method_module;

		rt_compiled_block_t method = rt_lookup(obj, method_name, &method_module);

		return method(0, method_name, method_module, obj, block, argc, argv);
	}

	rt_value __stdcall rt_support_super(rt_value method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
	{
		rt_value result_module;

		rt_compiled_block_t method = rt_lookup_super(method_module, method_name, &result_module);

		return method(0, method_name, result_module, obj, block, argc, argv);
	}
#endif

rt_value __cdecl rt_support_closure(struct rt_block *block, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[])
{
	rt_value self;

	__asm__ __volatile__("" : "=D" (self));

	rt_value closure = (rt_value)rt_alloc(sizeof(struct rt_proc) + sizeof(rt_value *) * argc);

	RT_COMMON(closure)->flags = C_PROC;
	RT_COMMON(closure)->class_of = rt_Proc;
	RT_COMMON(closure)->vars = 0;
	RT_PROC(closure)->self = self;
	RT_PROC(closure)->closure = block;
	RT_PROC(closure)->method_name = method_name;
	RT_PROC(closure)->method_module = method_module;
	RT_PROC(closure)->scope_count = argc;

	RT_ARG_EACH_RAW(i)
	{
		RT_PROC(closure)->scopes[i] = RT_ARG(i);
	}

	return closure;
}

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super)
{
	rt_value obj;

	__asm__ __volatile__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_class_symbol(obj, name, super);
}

rt_value __stdcall rt_support_define_module(rt_value name)
{
	rt_value obj;

	__asm__ __volatile__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	return rt_define_module_symbol(obj, name);
}

void __stdcall rt_support_define_method(rt_value name, struct rt_block *block)
{
	rt_value obj;

	__asm__ __volatile__("" : "=D" (obj));

	if(obj == rt_main)
		obj = rt_Object;

	#ifdef DEBUG
		printf("Defining method %s.%s\n", rt_string_to_cstr(rt_inspect(obj)), rt_symbol_to_cstr(name));
	#endif

	rt_class_set_method(obj, name, block);
}

rt_value rt_support_get_ivar(void)
{
	rt_value obj;
	rt_value name;

	__asm__ __volatile__("" : "=D" (obj), "=a" (name));

	#ifdef DEBUG
		printf("Looking up instance variable %s in %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)));
	#endif

	return rt_object_get_var(obj, name);
}

void __stdcall rt_support_set_ivar(rt_value value)
{
	rt_value obj;
	rt_value name;

	__asm__ __volatile__("" : "=D" (obj), "=a" (name));

	#ifdef DEBUG
		printf("Setting instance variable %s in %s to %s\n", rt_symbol_to_cstr(name), rt_string_to_cstr(rt_inspect(obj)), rt_string_to_cstr(rt_inspect(value)));
	#endif

	rt_object_set_var(obj, name, value);
}

#ifdef WIN_SEH
	static bool rt_find_seh_target(void *target)
	{
		struct rt_frame *frame;

		__asm__ __volatile__("movl %%fs:(0), %0\n" : "=r" (frame));

		while(true)
		{
			if(frame == (void *)-1)
				return false;

			if(frame->handler == rt_support_handler)
			{
				if(frame->block == target)
					return true;
			}

			frame = frame->prev;
		}
	}
#endif

void __stdcall __attribute__((noreturn)) rt_support_return(rt_value value, void *target)
{
	#ifdef WIN_SEH
		if(!rt_find_seh_target(target))
			RT_ASSERT(0);
	#endif

	struct rt_exception_data data;

	data.type = E_RETURN_EXCEPTION;
	data.payload[0] = (void *)target;
	data.payload[1] = (void *)value;

	rt_exception_raise(&data);
}

void __stdcall __attribute__((noreturn)) rt_support_break(rt_value value, void *target, size_t id)
{
	#ifdef WIN_SEH
		if(!rt_find_seh_target(target))
			RT_ASSERT(0);
	#endif

	struct rt_exception_data data;

	data.type = E_BREAK_EXCEPTION;
	data.payload[0] = (void *)target;
	data.payload[1] = (void *)value;
	data.payload[2] = (void *)id;

	rt_exception_raise(&data);
}

static void rt_global_unwind(struct rt_frame *target, struct rt_frame *top)
{
	#ifdef WIN_SEH
		extern void __stdcall RtlUnwind(
			PVOID TargetFrame,
			PVOID TargetIp,
			PEXCEPTION_RECORD ExceptionRecord,
			PVOID ReturnValue
		);

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
	#else
		struct rt_frame *frame = top;

		while(frame != target)
		{
			frame->handler(frame, 0, 0, true);
			frame = frame->prev;
		}
	#endif
}

static void rt_local_unwind(struct rt_frame *frame, struct exception_block *block, struct exception_block *target)
{
	frame->handling = 1;

	size_t ebp = (size_t)&frame->old_ebp;

	while(block != target)
	{
		if(block->ensure_label)
		{
			#ifdef DEBUG
				printf("Ensure block found\n");
				printf("Ebp: %x\n", ebp);
				printf("Eip: %x\n", (size_t)block->ensure_label);
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

	frame->handling = 0;
}

static void __attribute__((noreturn)) rt_handle_return(struct rt_frame *frame, struct rt_frame *top, struct exception_block *block, rt_value value)
{
	rt_global_unwind(frame, top);

	/*
	 * Execute frame local ensure handlers
	 */

	rt_local_unwind(frame, block, 0);

	/*
	 * Go to the handler and never return
	 */

	size_t ebp = (size_t)&frame->old_ebp;

	#ifdef DEBUG
		printf("Return target found\n");
		printf("Ebp: %x\n", ebp);
		printf("Esp: %x\n", ebp - 20 - 12 - frame->block->local_storage);
		printf("Eip: %x\n", (size_t)frame->block->epilog);
	#endif

	__asm__ __volatile__("mov %0, %%ecx\n"
		"mov %1, %%esp\n"
		"mov %2, %%ebp\n"
		"jmp *%%ecx\n"
	:
	: "r" (frame->block->epilog),
		"r" (ebp - 20 - 12 - frame->block->local_storage),
		"r" (ebp),
		"a" (value)
	: "%ecx");

	__builtin_unreachable();
}

static void __attribute__((noreturn)) rt_handle_break(struct rt_frame *frame, struct rt_frame *top, rt_value value, size_t id)
{
	rt_global_unwind(frame, top);

	/*
	 * Go to the handler and never return
	 */

	size_t ebp = (size_t)&frame->old_ebp;

	#ifdef DEBUG
		printf("Break target found\n");
		printf("Ebp: %x\n", ebp);
		printf("Esp: %x\n", ebp - 20 - 12 - frame->block->local_storage);
		printf("Eip: %x\n", (size_t)frame->block->break_targets[id]);
	#endif

	__asm__ __volatile__("mov %0, %%ecx\n"
		"mov %1, %%esp\n"
		"mov %2, %%ebp\n"
		"jmp *%%ecx\n"
	:
	: "r" (frame->block->break_targets[id]),
		"r" (ebp - 20 - 12 - frame->block->local_storage),
		"r" (ebp),
		"a" (value)
	: "%ecx");

	__builtin_unreachable();
}

static void __attribute__((noreturn)) rt_rescue(struct rt_frame *frame, struct rt_frame *top, struct exception_block *block, struct exception_block *current_block, void *rescue_label)
{
	rt_global_unwind(frame, top);

	/*
	 * Execute frame local ensure handlers
	 */

	rt_local_unwind(frame, block, current_block);

	/*
	 * Set to the current handler index to the parent
	 */

	frame->block_index = current_block->parent_index;

	/*
	 * Go to the handler and never return
	 */

	size_t ebp = (size_t)&frame->old_ebp;

	#ifdef DEBUG
		printf("Rescue block found\n");
		printf("Ebp: %x\n", ebp);
		printf("Esp: %x\n", ebp - 20 - 12 - frame->block->local_storage);
		printf("Eip: %x\n", (size_t)rescue_label);
	#endif

	__asm__ __volatile__("mov %0, %%eax\n"
		"mov %1, %%esp\n"
		"mov %2, %%ebp\n"
		"jmp *%%eax\n"
	:
	: "r" (rescue_label),
		"r" (ebp - 20 - 12 - frame->block->local_storage),
		"r" (ebp)
	: "%eax");

	__builtin_unreachable();
}

static void rt_handle_exception(struct rt_frame *frame, struct rt_frame *top, struct rt_exception_data *data, bool unwinding)
{
	if(frame->block_index == (size_t)-1) // Outside any exception block
	{
		if(!unwinding && data && data->payload[0] == frame->block)
		{
			switch(data->type)
			{
				case E_RETURN_EXCEPTION:
					rt_handle_return(frame, top, 0, (rt_value)data->payload[1]);
					break;

				case E_BREAK_EXCEPTION:
					rt_handle_break(frame, top, (size_t)data->payload[1], (rt_value)data->payload[2]);
					break;

				default:
					break;
			}
		}

		return;
	}

	struct exception_block *block = frame->block->exception_blocks[frame->block_index];

	if(unwinding)
	{
		rt_local_unwind(frame, block, 0);
	}
	else if(data)
	{
		switch(data->type)
		{
			case E_BREAK_EXCEPTION:
				rt_handle_break(frame, top, (size_t)data->payload[1], (rt_value)data->payload[2]);
				break;

			case E_RETURN_EXCEPTION:
				rt_handle_return(frame, top, block, (rt_value)data->payload[1]);
				break;

			case E_RUBY_EXCEPTION:
			{
				struct exception_block *current_block = block;

				while(current_block)
				{
					for(size_t i = 0; i < current_block->handlers.size; i++)
					{
						struct exception_handler *handler = current_block->handlers.array[i];

						switch(handler->type)
						{
							case E_RUNTIME_EXCEPTION:
								rt_rescue(frame, top, block, current_block, ((struct runtime_exception_handler *)handler)->rescue_label);

							default:
								break;
						}
					}

					current_block = current_block->parent;
				}
			}
			break;

			default:
                RT_ASSERT(0);
		}
	}
}

#ifdef WIN_SEH
	EXCEPTION_DISPOSITION __cdecl rt_support_handler(EXCEPTION_RECORD *exception, struct rt_frame *frame, CONTEXT *context, void *dispatcher_context)
	{
		struct rt_exception_data *data = 0;

		if(exception->ExceptionCode == RT_SEH_RUBY)
			data = (struct rt_exception_data *)exception->ExceptionInformation[0];

		rt_handle_exception(frame, 0, data, exception->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND));

		return ExceptionContinueSearch;
	}
#else
	void rt_support_handler(struct rt_frame *frame, struct rt_frame *top, struct rt_exception_data *data, bool unwinding)
	{
		rt_handle_exception(frame, top, data, unwinding);
	}
#endif
