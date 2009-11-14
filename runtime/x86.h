#pragma once
#include "../globals.h"
#include "../compiler/bytecode.h"
#include "../compiler/block.h"
#include "classes.h"
#include "support.h"

#ifdef WINDOWS
	#define RT_SEH_RUBY 0x4D520000

	struct rt_seh_frame {
		struct rt_seh_frame *prev;
		void *handler;
		size_t handling;
		size_t block_index;
		struct block_data *block;
		size_t old_ebp;
	};
#endif

rt_value __cdecl rt_support_closure(rt_compiled_block_t block, size_t argc, rt_value *argv[]);

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super);
rt_value __stdcall rt_support_define_module(rt_value name);

void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block);

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj);

rt_value rt_support_get_ivar(void);
void __stdcall rt_support_set_ivar(rt_value value);

#ifdef WINDOWS
	void __stdcall rt_support_return(rt_value value, void *target);
	void __stdcall rt_support_break(rt_value value, void *target, size_t id);

	EXCEPTION_DISPOSITION __cdecl rt_support_seh_handler(EXCEPTION_RECORD *exception, struct rt_seh_frame *frame_data, CONTEXT *context, void *dispatcher_context);
#else
	/*
	 * Dummy functions
	 */
	void rt_support_return(void);
	void rt_support_break(void);
	void rt_support_seh_handler(void);
#endif
