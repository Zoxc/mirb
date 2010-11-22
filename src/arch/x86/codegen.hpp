#pragma once
#include "x86.hpp"
#include "../../codegen/bytecode.hpp"
#include "../../codegen/opcodes.hpp"
#include "../../mem_stream.hpp"

namespace Mirb
{
	class Block;
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
				MemoryPool &memory_pool;

				Block *block;
				size_t index;
				
				size_t locals_offset;
				
				static const size_t handler_offset = -8;
				static const size_t handling_offset = -12;

				size_t reg_low(size_t reg);
				size_t reg_high(size_t reg);
				int stack_offset(Tree::Variable *var);
				int stack_offset(size_t loc);
				void stack_pop(size_t count);
				void rex(size_t r, size_t x, size_t b);
				void modrm(size_t mod, size_t reg, size_t rm);
				void stack_modrm(size_t reg, Tree::Variable *var);
				void mov_imm_to_reg(size_t imm, size_t reg);
				void mov_imm_to_var(size_t imm, Tree::Variable *var);
				void mov_reg_to_reg(size_t src, size_t dst);
				void mov_reg_to_var(size_t reg, Tree::Variable *var);
				void mov_var_to_reg(Tree::Variable *var, size_t reg);
				void push_reg(size_t reg);
				void push_var(Tree::Variable *var);
				void push_imm(size_t imm);
				void test_var(Tree::Variable *var);
				void mov_arg_to_reg(size_t arg, size_t reg);
				void mov_arg_to_var(size_t arg, Tree::Variable *var);
				void mov_reg_index_to_reg(size_t sreg, size_t index, size_t dreg);
				void mov_var_index_to_reg(Tree::Variable *var, size_t index, size_t reg);
				void mov_reg_to_reg_index(size_t sreg, size_t dreg, size_t index);
				void mov_reg_to_var_index(size_t reg, Tree::Variable *var, size_t index);
				
				template<typename T> void call(T *func)
				{
					stream.b(0xE8);
					stream.d((size_t)func - ((size_t)stream.position + 4));
				}

				SimplerList<BranchOpcode, BranchOpcode, &BranchOpcode::branch_entry> branch_list;
			public:
				NativeGenerator(MemStream &stream, MemoryPool &memory_pool) : stream(stream), memory_pool(memory_pool) {}
				
				static const size_t stub_size = 10;

				static const size_t long_mode = false;
				static const size_t spare = Arch::Register::AX;

				template<typename T> void generate(T &op)
				{
					debug_fail("Not implemented");
				}
				
				void generate(Block *block);
				void generate_stub(Mirb::Block *block);
		};
		
		template<> void NativeGenerator::generate(MoveOp &op);
		template<> void NativeGenerator::generate(LoadOp &op);
		template<> void NativeGenerator::generate(LoadRawOp &op);
		template<> void NativeGenerator::generate(LoadArgOp &op);
		template<> void NativeGenerator::generate(PushOp &op);
		template<> void NativeGenerator::generate(PushImmediateOp &op);
		template<> void NativeGenerator::generate(PushRawOp &op);
		template<> void NativeGenerator::generate(ClosureOp &op);
		template<> void NativeGenerator::generate(ClassOp &op);
		template<> void NativeGenerator::generate(ModuleOp &op);
		template<> void NativeGenerator::generate(CallOp &op);
		template<> void NativeGenerator::generate(SuperOp &op);
		template<> void NativeGenerator::generate(CreateHeapOp &op);
		template<> void NativeGenerator::generate(GetHeapOp &op);
		template<> void NativeGenerator::generate(GetHeapVarOp &op);
		template<> void NativeGenerator::generate(SetHeapVarOp &op);
		template<> void NativeGenerator::generate(GetIVarOp &op);
		template<> void NativeGenerator::generate(SetIVarOp &op);
		template<> void NativeGenerator::generate(GetConstOp &op);
		template<> void NativeGenerator::generate(SetConstOp &op);
		template<> void NativeGenerator::generate(BranchIfOp &op);
		template<> void NativeGenerator::generate(BranchUnlessOp &op);
		template<> void NativeGenerator::generate(BranchOp &op);
		template<> void NativeGenerator::generate(ReturnOp &op);
		template<> void NativeGenerator::generate(HandlerOp &op);
		template<> void NativeGenerator::generate(UnwindOp &op);
		template<> void NativeGenerator::generate(UnwindReturnOp &op);
		template<> void NativeGenerator::generate(UnwindBreakOp &op);
		template<> void NativeGenerator::generate(ArrayOp &op);
		template<> void NativeGenerator::generate(StringOp &op);
		template<> void NativeGenerator::generate(InterpolateOp &op);
		
		template<typename T> struct FlushRegisters { static const bool value = false; };
		
		template<> struct FlushRegisters<ClosureOp> { static const bool value = true; };
		template<> struct FlushRegisters<ClassOp> { static const bool value = true; };
		template<> struct FlushRegisters<ModuleOp> { static const bool value = true; };
		template<> struct FlushRegisters<MethodOp> { static const bool value = true; };
		template<> struct FlushRegisters<CallOp> { static const bool value = true; };
		template<> struct FlushRegisters<CreateHeapOp> { static const bool value = true; };
		template<> struct FlushRegisters<SuperOp> { static const bool value = true; };
		template<> struct FlushRegisters<GetIVarOp> { static const bool value = true; };
		template<> struct FlushRegisters<SetIVarOp> { static const bool value = true; };
		template<> struct FlushRegisters<GetConstOp> { static const bool value = true; };
		template<> struct FlushRegisters<SetConstOp> { static const bool value = true; };
		template<> struct FlushRegisters<ArrayOp> { static const bool value = true; };
		template<> struct FlushRegisters<StringOp> { static const bool value = true; };
		template<> struct FlushRegisters<InterpolateOp> { static const bool value = true; };

		template<typename T> struct CanRaiseException { static const bool value = false; };

		template<> struct CanRaiseException<ClassOp> { static const bool value = true; };
		template<> struct CanRaiseException<ModuleOp> { static const bool value = true; };
		template<> struct CanRaiseException<CallOp> { static const bool value = true; };
		template<> struct CanRaiseException<SuperOp> { static const bool value = true; };
	};
};

