#pragma once
#include "../common.hpp"
#include "../value.hpp"

namespace Mirb
{
	class Block;
	class Symbol;

	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		struct Opcode;
		class Block;
		class BasicBlock;
		
		class ByteCodePrinter
		{
			private:
				Block *block;

				std::string imm(Value imm);
				
				std::string imm(Symbol *imm)
				{
					return this->imm(Value(imm));
				};
				
				std::string label(BasicBlock *label);
				std::string raw(size_t imm);
				std::string print_block(Mirb::Block *block);
			public:
				ByteCodePrinter(Block *block) : block(block), highlight(0) {}

				Tree::Variable *highlight;
				
				std::string var(Tree::Variable *var);
				std::string opcode(Opcode *opcode);
				std::string print_basic_block(BasicBlock *block);
				std::string print();
		};
	};
};
