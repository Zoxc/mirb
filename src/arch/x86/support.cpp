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
			rt_compiled_block_t __fastcall jit_invoke(Block *block)
			{
				return Compiler::defered_compile(block);
			}

			rt_value closure_call(rt_compiled_block_t code, rt_value *scopes[], Symbol *method_name, rt_value method_module, rt_value obj, rt_value block, size_t argc, rt_value argv[])
			{
				rt_value result;

				__asm
				{
					push ebx
					push eax
					push ecx
					push edx

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

					pop ebx
					pop ecx
					pop eax
					pop ebx
				}

				return result;
			}

			__declspec(naked) void jit_stub(Block *code_block, rt_value obj, rt_value block, size_t argc, rt_value argv[])
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

					push [esp + 20]
					push [esp + 20]
					push [esp + 20]
					push [esp + 20]
					call ebx

					pop ebx
					add esp, 4
					ret 16
				}
			}

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

