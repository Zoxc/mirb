#pragma once
#include "../../runtime/runtime.hpp"
#include "../common.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		class Block;
		class BasicBlock;
		
		class DotPrinter
		{
			public:
				std::string print_link(BasicBlock *block, BasicBlock *next, bool regular = true);
				std::string print_basic_block(BasicBlock *block);
				void print_block(Block *block, std::string filename);
		};
	};
};
