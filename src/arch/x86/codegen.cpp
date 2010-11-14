#include "codegen.hpp"
#include "disassembly.hpp"
#include "../../tree/tree.hpp"
#include "../../mem_stream.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		size_t NativeMeasurer::measure(Block *block)
		{
			return 0x1000;
		}

		template<typename T> struct Generate
		{
			static void func(NativeGenerator &generator, T &opcode)
			{
				generator.generate<T>(opcode);
			}
		};

		void NativeGenerator::generate(Block *block)
		{
			index = 0;

			for(auto i = block->basic_blocks.begin(); i != block->basic_blocks.end(); ++i)
			{
				i().final = stream.position();

				for(auto o = i().opcodes.begin(); o != i().opcodes.end(); ++o)
				{
					o().virtual_do<Generate>(*this);
					index++;
				}
			}

			#ifdef DEBUG
				Arch::Disassembly::dump_code(stream, 0);
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
				if(var->reg)
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
				if(var->reg)
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

		int NativeGenerator::stack_offset(Tree::Variable *var)
		{
			return -(int)((1 + var->loc) * sizeof(size_t));
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
			if(var->reg)
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
			if(var->reg)
				mov_reg_to_reg(var->loc, reg);
			else
			{
				rex(reg_high(reg), 0, 0);
				stream.b(0x8B);
				stack_modrm(reg_low(reg), var);
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
		
		template<> void NativeGenerator::generate(MoveOp &op)
		{
			if(op.src->reg)
			{
				mov_reg_to_var(op.src->loc, op.dst);
			}
			else
			{
				if(op.dst->reg)
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
		
		template<> void NativeGenerator::generate(PushOp &op)
		{
			if(op.var->reg)
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

		template<> void NativeGenerator::generate(BranchOp &op)
		{
			push_imm(op.imm);
		}

		template<> void NativeGenerator::generate(ReturnOp &op)
		{
		}
	};
};

