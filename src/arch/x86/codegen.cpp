#include "codegen.hpp"
#include "disassembly.hpp"
#include "support.hpp"
#include "../../tree/tree.hpp"
#include "../../mem_stream.hpp"
#include "../../block.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		size_t NativeMeasurer::measure(Block *block)
		{
			return 0x1000; //TODO: Fix this...
		}

		void NativeGenerator::generate_stub(Mirb::Block *block)
		{
			push_imm((size_t)block);
			
			stream.b(0xE9);
			stream.d((size_t)Arch::Support::jit_stub - ((size_t)stream.position + 4));
		}

		template<typename T> struct Generate
		{
			// TODO: Figure out why things break when generator is a reference
			static void func(NativeGenerator *generator, T &opcode)
			{
				generator->generate<T>(opcode);
			}
		};

		void NativeGenerator::generate(Block *block)
		{
			this->block = block;

			push_reg(Arch::Register::BP);
			mov_reg_to_reg(Arch::Register::SP, Arch::Register::BP);
			
			if(block->scope->require_exceptions)
			{
				locals_offset = sizeof(Arch::Support::Frame) - sizeof(size_t);
				block->final->ebp_offset = locals_offset;

				// Store Arch::Support::Frame on stack
				
				// old_ebp is already pushed

				// block @ ebp - 4
				push_imm((size_t)block->final);

				// block_index @ ebp - 8
				push_imm((size_t)-1);

				// handling @ ebp - 12
				push_imm(0);

				// handler @ ebp - 16
				push_imm((size_t)&Arch::Support::exception_handler);

				#ifdef MIRB_SEH_EXCEPTIONS
					// prev @ ebp - 20
					stream.b(0x64); // push dword [fs:0]
					stream.b(0xFF);
					stream.b(0x35);
					stream.d(0);

					/*
					 * Append the struct to the linked list
					 */
					stream.b(0x64); // mov dword [fs:0], esp
					stream.b(0x89);
					stream.b(0x25);
					stream.d(0);
				#else
					// prev @ ebp - 20
					stream.b(0xFF); // push dword [Arch::Support::current_frame]
					modrm(0, 6, Arch::Register::BP);
					stream.d((size_t)&Arch::Support::current_frame);

					/*
					 * Append the struct to the linked list
					 */
					stream.b(0x89); // mov dword [Arch::Support::current_frame], esp
					modrm(0, Arch::Register::SP, Arch::Register::BP);
					stream.d((size_t)&Arch::Support::current_frame);
				#endif
			}
			else
				locals_offset = 0;
			
			if(block->stack_vars)
			{
				stream.b(0x81); stream.b(0xEC); // add esp,
				stream.d(block->stack_vars * sizeof(size_t));
				
				block->final->ebp_offset += block->stack_vars * sizeof(size_t);
			}
			
			if(BitSetWrapper<MemoryPool>::get(block->used_registers, Arch::Register::BX))
			{
				block->final->ebp_offset += sizeof(size_t);
				push_reg(Arch::Register::BX);
			}
			
			if(BitSetWrapper<MemoryPool>::get(block->used_registers, Arch::Register::SI))
			{
				block->final->ebp_offset += sizeof(size_t);
				push_reg(Arch::Register::SI);
			}
			
			if(BitSetWrapper<MemoryPool>::get(block->used_registers, Arch::Register::DI))
			{
				block->final->ebp_offset += sizeof(size_t);
				push_reg(Arch::Register::DI);
			}
			
			index = 0;

			for(auto i = block->basic_blocks.begin(); i != block->basic_blocks.end(); ++i)
			{
				i().final = stream.position;

				for(auto o = i().opcodes.begin(); o != i().opcodes.end(); ++o)
				{
					o().virtual_do<Generate>(this);
					index++;
				}
			}

			if(block->scope->require_exceptions)
			{
				block->final->epilog = stream.position;

				stream.b(0x8B); // mov ecx, dword [ebp - 20] ; Load previous exception frame
				stream.b(0x4D);
				stream.b(-20);

				#ifdef MIRB_SEH_EXCEPTIONS
					stream.b(0x64); // mov dword [fs:0], ecx
					stream.b(0x89);
					stream.b(0x0D);
					stream.d(0);	
				#else
					stream.b(0x89); // mov dword [Arch::Support::current_frame], ecx
					modrm(0, Arch::Register::CX, Arch::Register::BP);
					stream.d((size_t)&Arch::Support::current_frame);
				#endif
				
				for(auto i = block->final->exception_blocks.begin(); i != block->final->exception_blocks.end(); ++i)
				{
					if(i()->ensure_label.block)
						i()->ensure_label.address = i()->ensure_label.block->final;
					else
						i()->ensure_label.address = 0;
					
					for(auto j = i()->handlers.begin(); j != i()->handlers.end(); ++j)
					{
						switch(j()->type)
						{
							case RuntimeException:
							{
								RuntimeExceptionHandler *handler = (RuntimeExceptionHandler *)*j;
								
								handler->rescue_label.address = handler->rescue_label.block->final;
								
								break;
							}

							case ClassException:
							case FilterException:
							default:
								break;
						}
					}
				}
			}
			
			if(BitSetWrapper<MemoryPool>::get(block->used_registers, Arch::Register::DI))
				stream.b(0x5F); // pop edi
			
			if(BitSetWrapper<MemoryPool>::get(block->used_registers, Arch::Register::SI))
				stream.b(0x5E); // pop esi
			
			if(BitSetWrapper<MemoryPool>::get(block->used_registers, Arch::Register::BX))
				stream.b(0x5B); // pop ebx

			mov_reg_to_reg(Arch::Register::BP, Arch::Register::SP);
			stream.b(0x5D); // pop ebp
			
			stream.b(0xC2); // ret 16
			stream.w(16);
			

			// Update the branch targets
			
			for(auto o = branch_list.begin(); o != branch_list.end(); ++o)
			{
				size_t *addr = (size_t *)o().code;
				*addr = (size_t)o().label->final - ((size_t)addr + 4);
			}

			#ifdef DEBUG
				Vector<Arch::Disassembly::Symbol *, MemoryPool> symbols(memory_pool);

				Arch::Disassembly::Symbol final_block;
				Arch::Disassembly::Symbol final_block_owner;
				

				final_block.address = (void *)block->final;
				final_block.symbol = "Mirb::Block *block";

				symbols.push(&final_block);
				
				if(block->scope->owner != block->scope)
				{
					final_block_owner.address = (void *)block->scope->owner->final;
					final_block_owner.symbol = "Mirb::Block *owner";

					symbols.push(&final_block_owner);
				}

				Arch::Disassembly::dump_code(stream, &symbols);
			#endif
		}

		size_t NativeGenerator::reg_low(size_t reg)
		{
			return long_mode ? reg & 7 : reg;
		}

		size_t NativeGenerator::reg_high(size_t reg)
		{
			return reg >> 3;
		}

		void NativeGenerator::rex(size_t r, size_t x, size_t b)
		{
			if(long_mode)
				stream.b(0x40 | 8 | r << 2 | x << 1 | b);
		}
		
		void NativeGenerator::modrm(size_t mod, size_t reg, size_t rm)
		{
			stream.b(mod << 6 | reg << 3 | rm);
		}

		void NativeGenerator::stack_modrm(size_t reg, Tree::Variable *var)
		{
			modrm(1, reg, Arch::Register::BP);
			stream.b((int8_t)stack_offset(var));
		}

		void NativeGenerator::mov_imm_to_reg(size_t imm, size_t reg)
		{
			rex(0, 0, reg_high(reg));
			stream.b(0xB8 + reg_low(reg));
			stream.s(imm);
		}

		void NativeGenerator::mov_imm_to_var(size_t imm, Tree::Variable *var)
		{
			if(long_mode)
			{
				if(var->flags.get<Tree::Variable::Register>())
				{
					mov_imm_to_reg(imm, var->loc);
				}
				else
				{
					mov_imm_to_reg(imm, spare);
					mov_reg_to_var(spare, var);
				}
			}
			else
			{
				if(var->flags.get<Tree::Variable::Register>())
				{
					mov_imm_to_reg(imm, var->loc);
				}
				else
				{
					stream.b(0xC7);
					stack_modrm(0, var);
					stream.s(imm);
				}
			}
		}
		
		int NativeGenerator::stack_offset(size_t loc)
		{
			return -(int)((1 + loc) * sizeof(size_t) + locals_offset);
		}

		int NativeGenerator::stack_offset(Tree::Variable *var)
		{
			return stack_offset(var->loc);
		}

		void NativeGenerator::stack_pop(size_t count)
		{
			stream.b(0x81);
			stream.b(0xC4);
			stream.d(sizeof(size_t) * count);
		}

		void NativeGenerator::mov_reg_to_reg(size_t src, size_t dst)
		{
			if(src == dst)
				return;

			rex(reg_high(src), 0, reg_high(dst));
			stream.b(0x89);
			modrm(3, reg_low(src), reg_low(dst));
		}
		
		void NativeGenerator::mov_reg_to_var(size_t reg, Tree::Variable *var)
		{
			if(var->flags.get<Tree::Variable::Register>())
				mov_reg_to_reg(reg, var->loc);
			else
			{
				rex(reg_high(reg), 0, 0);
				stream.b(0x89);
				stack_modrm(reg_low(reg), var);
			}
		}
		
		void NativeGenerator::mov_var_to_reg(Tree::Variable *var, size_t reg)
		{
			if(var->flags.get<Tree::Variable::Register>())
				mov_reg_to_reg(var->loc, reg);
			else
			{
				rex(reg_high(reg), 0, 0);
				stream.b(0x8B);
				stack_modrm(reg_low(reg), var);
			}
		}

		void NativeGenerator::mov_arg_to_reg(size_t arg, size_t reg)
		{
			rex(reg_high(reg), 0, 0);
			stream.b(0x8B);
			modrm(1, reg, Arch::Register::BP);
			stream.b((int8_t)arg * sizeof(size_t) + 2 * sizeof(size_t));
		}

		void NativeGenerator::mov_arg_to_var(size_t arg, Tree::Variable *var)
		{
			if(var->flags.get<Tree::Variable::Register>())
				mov_arg_to_reg(arg, var->loc);
			else
			{
				mov_arg_to_reg(arg, spare);
				mov_reg_to_var(spare, var);
			}
		}

		void NativeGenerator::mov_reg_index_to_reg(size_t sreg, size_t index, size_t dreg)
		{
			rex(reg_high(dreg), 0, 0);
			stream.b(0x8B);
			
			if(index)
			{
				modrm(1, reg_low(dreg), sreg);
				stream.b((int8_t)index * sizeof(size_t));
			}
			else
				modrm(0, reg_low(dreg), sreg);
		}

		void NativeGenerator::mov_var_index_to_reg(Tree::Variable *var, size_t index, size_t reg)
		{
			if(var->flags.get<Tree::Variable::Register>())
			{
				mov_reg_index_to_reg(var->loc, index, reg);
			}
			else
			{
				mov_var_to_reg(var, spare);
				mov_reg_index_to_reg(spare, index, reg);
			}
		}
		
		void NativeGenerator::mov_reg_to_reg_index(size_t sreg, size_t dreg, size_t index)
		{
			rex(reg_high(sreg), 0, 0);
			stream.b(0x89);

			if(index)
			{
				modrm(1, reg_low(sreg), dreg);
				stream.b((int8_t)index * sizeof(size_t));
			}
			else
				modrm(0, reg_low(sreg), dreg);
		}

		void NativeGenerator::mov_reg_to_var_index(size_t reg, Tree::Variable *var, size_t index)
		{
			if(var->flags.get<Tree::Variable::Register>())
			{
				mov_reg_to_reg_index(reg, var->loc, index);
			}
			else
			{
				mov_var_to_reg(var, spare);
				mov_reg_to_reg_index(reg, spare, index);
			}
		}

		void NativeGenerator::push_reg(size_t reg)
		{
			rex(0, 0, reg_high(reg));
			stream.b(0x50 + reg_low(reg));
		}

		void NativeGenerator::push_imm(size_t imm)
		{
			if(long_mode)
			{
				mov_imm_to_reg(imm, spare);
				push_reg(spare);
			}
			else
			{
				stream.b(0x68);
				stream.s(imm);
			}
		}
		
		void NativeGenerator::push_var(Tree::Variable *var)
		{
			if(var->flags.get<Tree::Variable::Register>())
				push_reg(var->loc);
			else
			{
				rex(0, 0, 0);
				stream.b(0xFF);
				stack_modrm(6, var);
			}
		}

		void NativeGenerator::test_var(Tree::Variable *var)
		{
			mov_var_to_reg(var, spare);

			debug_assert(spare == Arch::Register::AX);

			rex(0, 0, 0);
			stream.b(0x83); // and eax, ~(RT_FALSE | RT_NIL)
			modrm(3, 4, Arch::Register::AX);
			stream.b((int8_t)(~(RT_FALSE | RT_NIL)));
		}
		
		template<> void NativeGenerator::generate(MoveOp &op)
		{
			if(op.src->flags.get<Tree::Variable::Register>())
			{
				mov_reg_to_var(op.src->loc, op.dst);
			}
			else
			{
				if(op.dst->flags.get<Tree::Variable::Register>())
					mov_var_to_reg(op.src, op.dst->loc);
				else
				{
					mov_var_to_reg(op.src, spare);
					mov_reg_to_var(spare, op.dst);
				}
			}
		}
		
		template<> void NativeGenerator::generate(LoadOp &op)
		{
			mov_imm_to_var(op.imm, op.var);
		}
		
		template<> void NativeGenerator::generate(LoadRawOp &op)
		{
			mov_imm_to_var(op.imm, op.var);
		}

		template<> void NativeGenerator::generate(LoadArgOp &op)
		{
			mov_arg_to_var(op.arg, op.var);
		}
		
		template<> void NativeGenerator::generate(PushOp &op)
		{
			if(op.var->flags.get<Tree::Variable::Register>())
				push_reg(op.var->loc);
			else
			{
				rex(0, 0, 0);
				stream.b(0x8B);
				stack_modrm(6, op.var);
			}
		}
		
		template<> void NativeGenerator::generate(PushImmediateOp &op)
		{
			push_imm(op.imm);
		}
		
		template<> void NativeGenerator::generate(PushRawOp &op)
		{
			push_imm(op.imm);
		}
		
		template<> void NativeGenerator::generate(ClosureOp &op)
		{
			push_reg(Arch::Register::SP);
			push_imm(op.scope_count);
			
			push_var(op.self);
			
			push_imm((size_t)op.block);

			call(&Arch::Support::create_closure);

			stack_pop(op.scope_count + 4);

			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(ClassOp &op)
		{
			if(op.super)
				push_var(op.super);
			else
				push_imm((size_t)rt_Object);
			
			push_imm((size_t)op.name);
			push_var(op.self);
			call(&Arch::Support::define_class);
			
			push_reg(Arch::Register::SP); // dummy
			push_imm(0);
			push_imm(0);
			push_reg(Arch::Register::AX); // obj
			call(op.block->compiled);
			
			if(op.var)
				mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(ModuleOp &op)
		{
			push_imm((size_t)op.name);
			push_var(op.self);
			call(&Arch::Support::define_module);
			
			push_reg(Arch::Register::SP); // dummy
			push_imm(0);
			push_imm(0);
			push_reg(Arch::Register::AX); // obj
			call(op.block->compiled);
			
			if(op.var)
				mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(MethodOp &op)
		{
			push_imm((size_t)op.block);
			push_imm((size_t)op.name);
			push_var(op.self);
			call(&Arch::Support::define_method);
		}
		
		template<> void NativeGenerator::generate(CallOp &op)
		{
			push_reg(Arch::Register::SP);
			push_imm(op.param_count);

			if(op.block)
				push_var(op.block);
			else
				push_imm(RT_NIL);
			
			push_var(op.obj);

			mov_imm_to_reg((size_t)op.method, Arch::Register::CX);

			call(&Arch::Support::call);

			stack_pop(op.param_count);
			
			if(op.break_id != Tree::InvokeNode::no_break_id)
				block->final->break_targets[op.break_id] = stream.position;

			if(op.var)
				mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(SuperOp &op)
		{
			push_reg(Arch::Register::SP);
			push_imm(op.param_count);

			if(op.block)
				push_var(op.block);
			else
				push_imm(RT_NIL);
			
			push_var(op.self);

			call(&Arch::Support::super);

			stack_pop(op.param_count);

			if(op.break_id != Tree::InvokeNode::no_break_id)
				block->final->break_targets[op.break_id] = stream.position;

			if(op.var)
				mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(LookupOp &op)
		{
			if(op.var->flags.get<Tree::Variable::Register>())
			{
				mov_var_index_to_reg(op.array_var, op.index, op.var->loc);
			}
			else
			{
				mov_var_index_to_reg(op.array_var, op.index, spare);
				mov_reg_to_var(spare, op.var);
			}
		}
		
		template<> void NativeGenerator::generate(CreateHeapOp &op)
		{
			push_imm(block->scope->heap_vars * sizeof(size_t));
			call(&Arch::Support::create_heap);

			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(GetHeapVarOp &op)
		{
			if(op.var->flags.get<Tree::Variable::Register>())
			{
				mov_var_index_to_reg(op.heap, op.index->loc, op.var->loc);
			}
			else
			{
				mov_var_index_to_reg(op.heap, op.index->loc, spare);
				mov_reg_to_var(spare, op.var);
			}
		}
		
		template<> void NativeGenerator::generate(SetHeapVarOp &op)
		{
			if(op.var->flags.get<Tree::Variable::Register>())
			{
				mov_reg_to_var_index(op.var->loc, op.heap, op.index->loc);
			}
			else
			{
				mov_var_to_reg(op.var, spare);
				mov_reg_to_var_index(spare, op.heap, op.index->loc);
			}
		}
		
		template<> void NativeGenerator::generate(GetIVarOp &op)
		{
			push_imm((size_t)op.name);
			push_var(op.self);
			
			call(&Arch::Support::get_ivar);

			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(SetIVarOp &op)
		{
			push_var(op.var);
			push_imm((size_t)op.name);
			push_var(op.self);
			
			call(&Arch::Support::set_ivar);
		}
		
		template<> void NativeGenerator::generate(GetConstOp &op)
		{
			push_imm((size_t)op.name);
			push_var(op.obj);
			
			call(&Arch::Support::get_const);

			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(SetConstOp &op)
		{
			push_var(op.var);
			push_imm((size_t)op.name);
			push_var(op.obj);
			
			call(&Arch::Support::set_const);
		}
		
		template<> void NativeGenerator::generate(BranchIfOp &op)
		{
			test_var(op.var);
			stream.b(0x0F);
			stream.b(0x85);
			op.code = stream.reserve(4);
			branch_list.append(&op);
		}
		
		template<> void NativeGenerator::generate(BranchUnlessOp &op)
		{
			test_var(op.var);
			stream.b(0x0F);
			stream.b(0x84);
			op.code = stream.reserve(4);
			branch_list.append(&op);
		}
		
		template<> void NativeGenerator::generate(BranchOp &op)
		{
			stream.b(0xE9);
			op.code = stream.reserve(4);
			branch_list.append(&op);
		}
		
		template<> void NativeGenerator::generate(ReturnOp &op)
		{
			mov_var_to_reg(op.var, Arch::Register::AX);
		}
		
		template<> void NativeGenerator::generate(HandlerOp &op)
		{
			// TODO: Use inc/dec where possible
			stream.b(0xC7);
			modrm(1, 0, Arch::Register::BP);
			stream.b((int8_t)handler_offset);
			stream.d(op.id);
		}
		
		template<> void NativeGenerator::generate(FlushOp &op)
		{
		}

		template<> void NativeGenerator::generate(UnwindOp &op)
		{
			stream.b(0x83); // cmp dword [ebp + handling_offset], 0
			modrm(1, 7, Arch::Register::BP);
			stream.b((int8_t)handling_offset);
			stream.b(0);

			stream.b(0x74); // jz +1
			stream.b(1);

			stream.b(0xC3); // ret
		}
		
		template<> void NativeGenerator::generate(UnwindReturnOp &op)
		{
			push_imm((size_t)op.code);
			push_var(op.var);
			call(&Arch::Support::far_return);
		}

		template<> void NativeGenerator::generate(UnwindBreakOp &op)
		{
			push_imm(op.index);
			push_imm((size_t)op.code);
			push_var(op.var);
			call(&Arch::Support::far_break);
		}

		template<> void NativeGenerator::generate(ArrayOp &op)
		{
			push_reg(Arch::Register::SP);
			push_imm(op.element_count);
			call(&Arch::Support::create_array);
			
			stack_pop(op.element_count + 2);
			
			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(StringOp &op)
		{
			push_imm((size_t)op.str);
			call(&Arch::Support::define_string);
			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
		template<> void NativeGenerator::generate(InterpolateOp &op)
		{
			push_reg(Arch::Register::SP);
			push_imm(op.param_count);
			call(&Arch::Support::interpolate);
			
			stack_pop(op.param_count + 2);
			
			mov_reg_to_var(Arch::Register::AX, op.var);
		}
		
	};
};

