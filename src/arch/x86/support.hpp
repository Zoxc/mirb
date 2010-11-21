#pragma once
#include "../../common.hpp"
#include "../../block.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Support
		{
			#ifdef _MSC_VER
				void jit_stub();
			#else
				extern void jit_stub() mirb_external("mirb_arch_support_jit_stub");
			#endif
			
			Value closure_call(compiled_block_t code, Value *scopes[], Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[]);
			
			Value *__stdcall create_heap(size_t bytes);
			Value __cdecl create_closure(Block *block, Value self, size_t argc, Value *argv[]);
			Value __cdecl create_array(size_t argc, Value argv[]);
			Value __cdecl interpolate(size_t argc, Value argv[]);
			
			Value __stdcall get_const(Value obj, Symbol *name);
			void __stdcall set_const(Value obj, Symbol *name, Value value);
			
			Value __stdcall get_ivar(Value obj, Symbol *name);
			void __stdcall set_ivar(Value obj, Symbol *name, Value value);

			Value __stdcall define_class(Value obj, Value name, Value super);
			Value __stdcall define_module(Value obj, Value name);
			void __stdcall define_method(Value obj, Value name, Block *block);
			Value __stdcall define_string(const char *string);

			Value __fastcall call(Symbol *method_name, Value dummy, Value obj, Value block, size_t argc, Value argv[]);
			Value __fastcall super(Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[]);
		};
	};
};
