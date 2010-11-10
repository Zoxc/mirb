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
		class Block;
		class BasicBlock;
		class ByteCodePrinter;
		
		class DotPrinter
		{
			public:
				DotPrinter() : highlight(0) {}

				Tree::Variable *highlight;

				std::string print_link(BasicBlock *block, BasicBlock *next, bool regular = true);
				std::string print_basic_block(ByteCodePrinter &printer, Block *main_block, BasicBlock *block, size_t &loc);
				void print_block(Block *block, std::string filename);
		};
	};
};
