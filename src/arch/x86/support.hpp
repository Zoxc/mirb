#pragma once
#include "../../common.hpp"
#include "../../block.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Support
		{
			struct Frame;
			
			#ifdef MIRB_SEH_EXCEPTIONS
				typedef EXCEPTION_DISPOSITION( __cdecl *rt_exception_handler_t)(EXCEPTION_RECORD *exception, Frame *frame, CONTEXT *context, void *dispatcher_context);

				EXCEPTION_DISPOSITION __cdecl exception_handler(EXCEPTION_RECORD *exception, Frame *frame, CONTEXT *context, void *dispatcher_context);
			#else
				typedef void (*exception_handler_t)(Frame *frame, Frame *top, ExceptionData *data, bool unwinding);

				void exception_handler(Frame *frame, Frame *top, ExceptionData *data, bool unwinding);
			#endif

			struct Frame {
				Frame *prev;
				exception_handler_t handler;
				size_t handling;
				size_t block_index;
				Mirb::Block *block;
				size_t old_ebp;
			};

			void __noreturn exception_raise(ExceptionData *data);

			extern Frame *current_frame;

			void __noreturn __stdcall far_return(rt_value value, Block *target);
			void __noreturn __stdcall far_break(rt_value value, Block *target, size_t id);

			#ifdef _MSC_VER
				void jit_stub();
			#else
				extern void jit_stub() mirb_external("mirb_arch_support_jit_stub");
			#endif
			
			value_t closure_call(compiled_block_t code, value_t *scopes[], Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[]);
			
			value_t *__stdcall create_heap(size_t bytes);
			value_t __cdecl create_closure(Block *block, value_t self, size_t argc, value_t *argv[]);
			value_t __cdecl create_array(size_t argc, value_t argv[]);
			value_t __cdecl interpolate(size_t argc, value_t argv[]);
			
			value_t __stdcall get_const(value_t obj, Symbol *name);
			void __stdcall set_const(value_t obj, Symbol *name, value_t value);
			
			value_t __stdcall get_ivar(value_t obj, Symbol *name);
			void __stdcall set_ivar(value_t obj, Symbol *name, value_t value);

			value_t __stdcall define_class(value_t obj, value_t name, value_t super);
			value_t __stdcall define_module(value_t obj, value_t name);
			void __stdcall define_method(value_t obj, value_t name, Block *block);
			value_t __stdcall define_string(const char *string);

			value_t __fastcall call(Symbol *method_name, value_t dummy, value_t obj, value_t block, size_t argc, value_t argv[]);
			value_t __fastcall super(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[]);
		};
	};
};
