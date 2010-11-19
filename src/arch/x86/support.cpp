#include "support.hpp"
#include "../../support.hpp"
#include "../../compiler.hpp"
#include "../../../runtime/classes/proc.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Support
		{
			rt_value __stdcall define_class(rt_value obj, rt_value name, rt_value super)
			{
				return Mirb::Support::define_class(obj, name, super);
			}
			
			rt_value __stdcall define_module(rt_value obj, rt_value name)
			{
				return Mirb::Support::define_module(obj, name);
			}
			
			void __stdcall define_method(rt_value obj, rt_value name, Block *block)
			{
				return Mirb::Support::define_method(obj, name, block);
			}
			
			rt_value __stdcall define_string(const char *string)
			{
				return Mirb::Support::define_string(string);
			}
				
			rt_value __cdecl interpolate(size_t argc, rt_value argv[])
			{
				return Mirb::Support::interpolate(argc, argv);
			}
			
			rt_value __cdecl create_array(size_t argc, rt_value argv[])
			{
				return Mirb::Support::create_array(argc, argv);
			}
			
			rt_compiled_block_t __fastcall jit_invoke(Block *block) mirb_external("mirb_arch_support_jit_invoke");

			rt_compiled_block_t __fastcall jit_invoke(Block *block)
			{
				return Compiler::defered_compile(block);
			}

			#ifdef _MSC_VER
				rt_value closure_call(rt_compiled_block_t code, rt_value *scopes[], Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
				{
					rt_value result;

					__asm
					{
						push argv
						push argc
						push block
						push obj
						mov eax, scopes
						mov ecx, method_name
						mov edx, method_module
						mov ebx, code
						call ebx

						mov result, eax
					}

					return result;
				}

				__declspec(naked) void jit_stub()
				{
					__asm
					{
						push ebx

						push eax
						push ecx
						push edx
						mov ecx, [esp + 16]
						call jit_invoke
						mov ebx, eax
						pop edx
						pop ecx
						pop eax

						push [esp + 24]
						push [esp + 24]
						push [esp + 24]
						push [esp + 24]
						call ebx

						pop ebx
						add esp, 4
						ret 16
					}
				}
			#else
				rt_value closure_call(rt_compiled_block_t code, rt_value *scopes[], Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
				{
					rt_value result;
					
					typedef rt_value (__stdcall __attribute__((__regparm__(3))) *closure_block_t)(rt_value *scopes[], rt_value method_module, Symbol *method_name, rt_value obj, rt_value block, size_t argc, rt_value argv[]);
					
					closure_block_t closure_code = (closure_block_t)code;
					
					return closure_code(scopes, method_module, method_name, obj, block, argc, argv);
				}
			#endif
			
			rt_value __cdecl create_closure(Block *block, rt_value self, rt_value method_name, rt_value method_module, size_t argc, rt_value *argv[])
			{
				return Mirb::Support::create_closure(block, self, method_name, method_module, argc, argv);
			}

			rt_value __fastcall call(Symbol *method_name, rt_value dummy, rt_value obj, rt_value block, size_t argc, rt_value argv[])
			{
				return Mirb::Support::call(method_name, obj, block, argc, argv);
			}

			rt_value __fastcall super(Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
			{
				return Mirb::Support::super(method_name, method_module, obj, block, argc, argv);
			}
		};
	};
};

