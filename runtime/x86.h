#pragma once
#include "../globals.h"
#include "../compiler/bytecode.h"
#include "../compiler/block.h"
#include "classes.h"
#include "support.h"

#ifdef WINDOWS
	#define RT_SEH_RUBY 0x4D520000

	typedef struct seh_frame_t {
		struct seh_frame_t *prev;
		void *handler;
		size_t handling;
		size_t block_index;
		block_data_t *block;
		size_t old_ebp;
	} rt_seh_frame_t;
#endif

rt_value rt_support_closure(rt_compiled_block_t block, size_t argc, rt_upval_t *argv[]);

rt_value __stdcall rt_support_define_class(rt_value name, rt_value super);
rt_value __stdcall rt_support_define_module(rt_value name);

void __stdcall rt_support_define_method(rt_value name, rt_compiled_block_t block);

rt_compiled_block_t __cdecl rt_support_lookup_method(rt_value obj);
rt_upval_t *rt_support_upval_create(void);

rt_value rt_support_get_ivar(void);
void __stdcall rt_support_set_ivar(rt_value value);

rt_value rt_support_get_upval(void);
void __stdcall rt_support_set_upval(rt_value value);

#ifdef WINDOWS
	void __stdcall rt_support_return(rt_value value, void *target);
	void __stdcall rt_support_break(rt_value value, void *target, size_t id);

	EXCEPTION_DISPOSITION __cdecl rt_support_seh_handler(EXCEPTION_RECORD *exception, rt_seh_frame_t *frame_data, CONTEXT *context, void *dispatcher_context);
#endif
