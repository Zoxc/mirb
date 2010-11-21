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
			Value *__stdcall create_heap(size_t bytes)
			{
				return Mirb::Support::create_heap(bytes);
			}

			Value __stdcall define_class(Value obj, Value name, Value super)
			{
				return Mirb::Support::define_class(obj, name, super);
			}
			
			Value __stdcall define_module(Value obj, Value name)
			{
				return Mirb::Support::define_module(obj, name);
			}
			
			void __stdcall define_method(Value obj, Value name, Block *block)
			{
				return Mirb::Support::define_method(obj, name, block);
			}
			
			Value __stdcall define_string(const char *string)
			{
				return Mirb::Support::define_string(string);
			}
				
			Value __cdecl interpolate(size_t argc, Value argv[])
			{
				return Mirb::Support::interpolate(argc, argv);
			}
			
			Value __stdcall get_const(Value obj, Symbol *name)
			{
				return Mirb::Support::get_const(obj, name);
			}

			void __stdcall set_const(Value obj, Symbol *name, Value value)
			{
				return Mirb::Support::set_const(obj, name, value);
			}
			
			Value __stdcall get_ivar(Value obj, Symbol *name)
			{
				return Mirb::Support::get_ivar(obj, name);
			}

			void __stdcall set_ivar(Value obj, Symbol *name, Value value)
			{
				return Mirb::Support::set_ivar(obj, name, value);
			}

			Value __cdecl create_array(size_t argc, Value argv[])
			{
				return Mirb::Support::create_array(argc, argv);
			}
			
			compiled_block_t __fastcall jit_invoke(Block *block) mirb_external("mirb_arch_support_jit_invoke");

			compiled_block_t __fastcall jit_invoke(Block *block)
			{
				return Compiler::defered_compile(block);
			}

			#ifdef _MSC_VER
				Value closure_call(compiled_block_t code, Value *scopes[], Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[])
				{
					Value result;

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
				Value closure_call(compiled_block_t code, Value *scopes[], Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[])
				{
					typedef Value (__stdcall __attribute__((__regparm__(3))) *closure_block_t)(Value *scopes[], Value method_module, Symbol *method_name, Value obj, Value block, size_t argc, Value argv[]);
					
					closure_block_t closure_code = (closure_block_t)code;
					
					return closure_code(scopes, method_module, method_name, obj, block, argc, argv);
				}
			#endif
			
			Value __cdecl create_closure(Block *block, Value self, size_t argc, Value *argv[])
			{
				return Mirb::Support::create_closure(block, self, argc, argv);
			}

			Value __fastcall call(Symbol *method_name, Value dummy, Value obj, Value block, size_t argc, Value argv[])
			{
				return Mirb::Support::call(method_name, obj, block, argc, argv);
			}

			Value __fastcall super(Symbol *method_name, Value method_module, Value obj, Value block, size_t argc, Value argv[])
			{
				return Mirb::Support::super(method_name, method_module, obj, block, argc, argv);
			}
		};
	};
};

