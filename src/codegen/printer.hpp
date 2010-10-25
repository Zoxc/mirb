#pragma once
#include "../../runtime/runtime.hpp"
#include "../common.hpp"

namespace Mirb
{
	namespace Tree
	{
		class Variable;
	};
	
	namespace CodeGen
	{
		class Opcode;
		class Block;
		class Label;
		
		class ByteCodePrinter
		{
			private:
				std::string var(Tree::Variable *var);
				std::string imm(rt_value imm);
				
				std::string imm(Symbol *imm)
				{
					return this->imm((rt_value)imm);
				};
				
				std::string label(Label *label);
				std::string raw(size_t imm);
				std::string block(Block *block);
				std::string opcode(Opcode *opcode);
			public:
				std::string print_block(Block *block);
		};
	};
};
