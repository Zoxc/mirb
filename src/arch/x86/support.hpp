#pragma once
#include "../../common.hpp"
#include "../../block.hpp"
#include "../../../runtime/classes.hpp"

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
			
			rt_value closure_call(rt_compiled_block_t code, rt_value *scopes[], Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);

			rt_value __cdecl create_closure(Block *block, rt_value self, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[]);
			rt_value __cdecl create_array(size_t argc, rt_value argv[]);
			rt_value __cdecl interpolate(size_t argc, rt_value argv[]);
			
			rt_value __stdcall get_const(rt_value obj, Symbol *name);
			void __stdcall set_const(rt_value obj, Symbol *name, rt_value value);
			
			rt_value __stdcall get_ivar(rt_value obj, Symbol *name);
			void __stdcall set_ivar(rt_value obj, Symbol *name, rt_value value);

			rt_value __stdcall define_class(rt_value obj, rt_value name, rt_value super);
			rt_value __stdcall define_module(rt_value obj, rt_value name);
			void __stdcall define_method(rt_value obj, rt_value name, Block *block);
			rt_value __stdcall define_string(const char *string);

			rt_value __fastcall call(Symbol *method_name, rt_value dummy, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
			rt_value __fastcall super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
		};
	};
};
