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
			
			typedef void (*exception_handler_t)(Frame *frame, Frame *top, ExceptionData *data, bool unwinding);

			void exception_handler(Frame *frame, Frame *top, ExceptionData *data, bool unwinding);
			
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

			void __noreturn __stdcall far_return(value_t value, Block *target);
			void __noreturn __stdcall far_break(value_t value, Block *target, size_t id);

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

			value_t __stdcall define_class(value_t obj, Symbol *name, value_t super);
			value_t __stdcall define_module(value_t obj, Symbol *name);
			void __stdcall define_method(value_t obj, Symbol *name, Block *block);
			value_t __stdcall define_string(const char *string);
			
			compiled_block_t __cdecl lookup(value_t obj, Symbol *name, value_t *result_module) mirb_external("mirb_lookup");
			compiled_block_t __cdecl lookup_super(value_t module, Symbol *name, value_t *result_module) mirb_external("mirb_lookup_super");

			value_t __fastcall call(value_t block, value_t dummy, value_t obj, Symbol *method_name, size_t argc, value_t argv[]);
			value_t __fastcall super(value_t block, value_t method_module, value_t obj, Symbol *method_name, size_t argc, value_t argv[]);
		};
	};
};
