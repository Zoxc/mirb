#pragma once
#include "x86.hpp"
#include "../../codegen/bytecode.hpp"
#include "../../codegen/opcodes.hpp"

namespace Mirb
{
	class MemStream;
	class MemoryPool;
	
	namespace Tree
	{
		class Variable;
	};

	namespace CodeGen
	{
		class Block;

		class NativeMeasurer
		{
			public:
				static size_t measure(Block *block);
		};

		class NativeGenerator
		{
			private:
				MemStream &stream;
				size_t index;

				size_t reg_low(size_t reg);
				size_t reg_high(size_t reg);
				int stack_offset(Tree::Variable *var);
				void rex(size_t r, size_t x, size_t b);
				void modrm(size_t mod, size_t reg, size_t rm);
				void stack_modrm(size_t reg, Tree::Variable *var);
				void mov_imm_to_reg(size_t imm, size_t reg);
				void mov_imm_to_var(size_t imm, Tree::Variable *var);
				void mov_reg_to_reg(size_t src, size_t dst);
				void mov_reg_to_var(size_t reg, Tree::Variable *var);
				void mov_var_to_reg(Tree::Variable *var, size_t reg);
				void push_reg(size_t reg);
				void push_imm(size_t imm);
			public:
				NativeGenerator(MemStream &stream, MemoryPool &memory_pool) : stream(stream) {}

				static const size_t long_mode = false;
				static const size_t spare = Arch::Register::DI;

				template<typename T> void generate(T &op)
				{
					debug_fail("Not implemented");
				}

				void generate(Block *block);
		};
		
		template<> void NativeGenerator::generate(MoveOp &op);
		template<> void NativeGenerator::generate(LoadOp &op);
		template<> void NativeGenerator::generate(LoadRawOp &op);
		template<> void NativeGenerator::generate(PushOp &op);
		template<> void NativeGenerator::generate(PushImmediateOp &op);
		template<> void NativeGenerator::generate(PushRawOp &op);
		template<> void NativeGenerator::generate(BranchOp &op);
		template<> void NativeGenerator::generate(ReturnOp &op);
	};
};

