#include "support.hpp"
#include "../../support.hpp"
#include "../../compiler.hpp"
#include "../../runtime.hpp"
#include "../../classes/proc.hpp"

namespace Mirb
{
	namespace Arch
	{
		namespace Support
		{
			Frame *current_frame;

			value_t *__stdcall create_heap(size_t bytes)
			{
				return Mirb::Support::create_heap(bytes);
			}

			value_t __stdcall define_class(value_t obj, Symbol *name, value_t super)
			{
				return Mirb::Support::define_class(obj, name, super);
			}
			
			value_t __stdcall define_module(value_t obj, Symbol *name)
			{
				return Mirb::Support::define_module(obj, name);
			}
			
			void __stdcall define_method(value_t obj, Symbol *name, Block *block)
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

						push [esp + 24] // argv
						push [esp + 24] // argc
						push [esp + 24] // block
						push [esp + 24] // obj
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
			
			compiled_block_t __cdecl lookup(value_t obj, Symbol *name, value_t *result_module)
			{
				return Mirb::lookup(obj, name, result_module);
			}

			compiled_block_t __cdecl lookup_super(value_t module, Symbol *name, value_t *result_module)
			{
				return Mirb::lookup_super(module, name, result_module);
			}

			value_t __fastcall call(Symbol *method_name, value_t dummy, value_t obj, value_t block, size_t argc, value_t argv[])
			{
				return Mirb::Support::call(method_name, obj, block, argc, argv);
			}

			value_t __fastcall super(Symbol *method_name, value_t method_module, value_t obj, value_t block, size_t argc, value_t argv[])
			{
				return Mirb::Support::super(method_name, method_module, obj, block, argc, argv);
			}
			
			void __noreturn exception_raise(ExceptionData *data)
			{
				Frame *top = current_frame;
				Frame *frame = top;

				while(frame)
				{
					frame->handler(frame, top, data, false);
					frame = frame->prev;
				}

				mirb_runtime_abort("Unable to find a exception handler");
			}

			void __noreturn __stdcall far_return(value_t value, Block *target)
			{
				ExceptionData data;

				data.type = ReturnException;
				data.target = target;
				data.value = value;

				exception_raise(&data);
			}

			void __noreturn __stdcall far_break(value_t value, Block *target, size_t id)
			{
				ExceptionData data;

				data.type = BreakException;
				data.target = target;
				data.value = value;
				data.id = id;

				exception_raise(&data);
			}

			static void global_unwind(Frame *target, Frame *top)
			{
				Frame *frame = top;

				while(frame != target)
				{
					frame->handler(frame, 0, 0, true);
					frame = frame->prev;
				}
			}

			static void local_unwind(Frame *frame, ExceptionBlock *block, ExceptionBlock *target)
			{
				frame->handling = 1;

				size_t ebp_value = (size_t)&frame->old_ebp;
				
				while(block != target)
				{
					void *target_label = block->ensure_label.address;

					frame->block_index = block->parent_index;

					if(target_label)
					{
						#ifdef DEBUG
							printf("Ensure block found\n");
							printf("Ebp: %x\n", ebp_value);
							printf("Eip: %x\n", (size_t)target_label);
						#endif

						#ifdef _MSC_VER
							__asm
							{
								push ebx
								push esi
								push edi
								push ebp

								mov eax, target_label
								mov ebp, ebp_value
								call eax
								
								pop ebp
								pop edi
								pop esi
								pop ebx
							}
						#else
							__asm__ volatile("pushl %%ebp\n"
								"pushl %%ebx\n"
								"mov %%eax, %%ebp\n"
								"call *%%ecx\n"
								"popl %%ebx\n"
								"popl %%ebp\n"
							:
							: "a" (ebp_value), "c" (target_label)
							: "esi", "edi", "edx", "memory");
						#endif
					}

					block = block->parent;
				}

				frame->handling = 0;
			}
			
			static void __noreturn jump_to_handler(Frame *frame, void *target, value_t value)
			{
				current_frame = frame;

				size_t ebp_value = (size_t)&frame->old_ebp;
				size_t esp_value = ebp_value - frame->block->ebp_offset;

				#ifdef DEBUG
					printf("Jump target found\n");
					printf("Ebp: %x\n", ebp_value);
					printf("Esp: %x\n", esp_value);
					printf("Eip: %x\n", (size_t)target);
				#endif
				
				#ifdef _MSC_VER
					__asm
					{
						mov eax, value
						mov ebx, ebp_value
						mov ecx, esp_value
						mov edx, target
						
						mov ebp, ebx
						mov esp, ecx
						jmp edx
					}
				#else
					__asm__ __volatile__("mov %0, %%ecx\n"
						"mov %1, %%esp\n"
						"mov %2, %%ebp\n"
						"jmp *%%ecx\n"
					:
					: "r" (target),
						"r" (esp_value),
						"r" (ebp_value),
						"a" (value)
					: "%ecx");
				#endif
				__builtin_unreachable();
			}

			static void __noreturn handle_return(Frame *frame, Frame *top, ExceptionBlock *block, value_t value)
			{
				global_unwind(frame, top);

				/*
				 * Execute frame local ensure handlers
				 */

				local_unwind(frame, block, 0);
				
				/*
				 * Set to the current handler index to the parent
				 */

				frame->block_index = block->parent_index;
				
				/*
				 * Go to the handler and never return
				 */

				jump_to_handler(frame, frame->block->epilog, value);
			}

			static void __noreturn handle_break(Frame *frame, Frame *top, value_t value, size_t id)
			{
				global_unwind(frame, top);

				/*
				 * Go to the handler and never return
				 */

				jump_to_handler(frame, frame->block->break_targets[id], value);
			}

			static void __noreturn rescue(Frame *frame, Frame *top, ExceptionBlock *block, ExceptionBlock *current_block, void *rescue_label)
			{
				global_unwind(frame, top);

				/*
				 * Execute frame local ensure handlers
				 */

				local_unwind(frame, block, current_block);

				/*
				 * Set to the current handler index to the parent
				 */

				frame->block_index = current_block->parent_index;

				/*
				 * Go to the handler and never return
				 */

				jump_to_handler(frame, rescue_label, 0);
			}
			
			static void handle_exception(Frame *frame, Frame *top, ExceptionData *data, bool unwinding)
			{
				if(frame->block_index == (size_t)-1) // Outside any exception block
				{
					if(!unwinding && data && data->target == frame->block)
					{
						switch(data->type)
						{
							case ReturnException:
								handle_return(frame, top, 0, data->value);
								break;

							case BreakException:
								handle_break(frame, top, data->value, data->id);
								break;

							default:
								break;
						}
					}

					return;
				}

				ExceptionBlock *block = frame->block->exception_blocks[frame->block_index];

				if(unwinding)
				{
					local_unwind(frame, block, 0);
				}
				else if(data)
				{
					switch(data->type)
					{
						case BreakException:
							handle_break(frame, top,  data->value, data->id);
							break;

						case ReturnException:
							handle_return(frame, top, block, data->value);
							break;

						case RubyException:
						{
							ExceptionBlock *current_block = block;

							while(current_block)
							{
								for(auto i = current_block->handlers.begin(); i != current_block->handlers.end(); ++i)
								{
									switch(i()->type)
									{
										case RuntimeException:
											rescue(frame, top, block, current_block, ((RuntimeExceptionHandler *)i())->rescue_label.address);

										default:
											break;
									}
								}

								current_block = current_block->parent;
							}
						}
						break;

						default:
							mirb_debug_abort("Unknown exception type");
					}
				}
			}
			
			void exception_handler(Frame *frame, Frame *top, ExceptionData *data, bool unwinding)
			{
				handle_exception(frame, top, data, unwinding);
			}
		};
	};
};

