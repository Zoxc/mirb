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
			value_t *__stdcall create_heap(size_t bytes)
			{
				return Mirb::Support::create_heap(bytes);
			}

			value_t __stdcall define_class(value_t obj, value_t name, value_t super)
			{
				return Mirb::Support::define_class(obj, name, super);
			}
			
			value_t __stdcall define_module(value_t obj, value_t name)
			{
				return Mirb::Support::define_module(obj, name);
			}
			
			void __stdcall define_method(value_t obj, value_t name, Block *block)
			{
				return Mirb::Support::define_method(obj, name, block);
			}
			
			value_t __stdcall define_string(const char *string)
			{
				return Mirb::Support::define_string(string);
			}
				
			value_t __cdecl interpolate(size_t argc, value_t argv[])
			{
				return Mirb::Support::interpolate(argc, argv);
			}
			
			value_t __stdcall get_const(value_t obj, Symbol *name)
			{
				return Mirb::Support::get_const(obj, name);
			}

			void __stdcall set_const(value_t obj, Symbol *name, value_t value)
			{
				return Mirb::Support::set_const(obj, name, value);
			}
			
			value_t __stdcall get_ivar(value_t obj, Symbol *name)
			{
				return Mirb::Support::get_ivar(obj, name);
			}

			void __stdcall set_ivar(value_t obj, Symbol *name, value_t value)
			{
				return Mirb::Support::set_ivar(obj, name, value);
			}

			value_t __cdecl create_array(size_t argc, value_t argv[])
			{
				return Mirb::Support::create_array(argc, argv);
			}
			
			compiled_block_t __fastcall jit_invoke(Block *block) mirb_external("mirb_arch_support_jit_invoke");

			compiled_block_t __fastcall jit_invoke(Block *block)
			{
				return Compiler::defered_compile(block);
			}

			#ifdef _MSC_VER
				value_t closure_call(compiled_block_t code, value_t *scopes[], Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])
				{
					value_t result;

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
				value_t closure_call(compiled_block_t code, value_t *scopes[], Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])
				{
					typedef value_t (__stdcall __attribute__((__regparm__(3))) *closure_block_t)(value_t *scopes[], value_t method_module, Symbol *method_name, value_t obj, value_t block, size_t argc, value_t argv[]);
					
					closure_block_t closure_code = (closure_block_t)code;
					
					return closure_code(scopes, method_module, method_name, obj, block, argc, argv);
				}
			#endif
			
			value_t __cdecl create_closure(Block *block, value_t self, size_t argc, value_t *argv[])
			{
				return Mirb::Support::create_closure(block, self, argc, argv);
			}

			value_t __fastcall call(Symbol *method_name, value_t dummy, value_t obj, value_t block, size_t argc, value_t argv[])
			{
				return Mirb::Support::call(method_name, obj, block, argc, argv);
			}

			value_t __fastcall super(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])
			{
				return Mirb::Support::super(method_name, method_module, obj, block, argc, argv);
			}
		};
	};
};

