#pragma once
#include "../common.hpp"
#include "../value.hpp"
#include "opcodes.hpp"

namespace Mirb
{
	class Block;
	class Symbol;

	namespace Tree
	{
		class Variable;
		class NamedVariable;
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
				BasicBlock *basic_block;
				const char *data;

				std::string imm(Symbol *imm);
				std::string label(const char *opcode);
				std::string label(BasicBlock *label);
				std::string raw(size_t imm);
				std::string print_block(Mirb::Block *block);
			public:
				ByteCodePrinter(Block *block) : block(block), highlight(0) {}

				Tree::Variable *highlight;
				
				std::string var(var_t var);
				std::string var_name(Tree::NamedVariable *var);
				std::string opcode(const char *&opcode);
				std::string print_basic_block(BasicBlock *block);
				std::string print();
		};
	};
};
