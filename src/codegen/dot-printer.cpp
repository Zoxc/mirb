#include <fstream>
#include "dot-printer.hpp"
#include "printer.hpp"
#include "opcodes.hpp"
#include "block.hpp"
#include "../tree/tree.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		std::string DotPrinter::print_link(BasicBlock *block, BasicBlock *next, bool regular)
		{
			std::stringstream result;

			result << "B" << block << ":";
			
			if(block->opcodes.last)
				result << "C" << block->opcodes.last;
			else
				result << "header";

			result << " -> " << "B" << next << ":header";

			if(highlight)
				if(BitSetWrapper<MemoryPool>::get(next->in, highlight->index))
					result << "[color=dodgerblue2]";

			result << ";\n";

			return result.str();
		}

		std::string DotPrinter::print_basic_block(Block *main_block, BasicBlock *block, size_t &loc)
		{
			ByteCodePrinter printer;

			printer.highlight = highlight;
			
			std::stringstream result;

			result << "B" << block << " [fontname=Courier New] [fontsize=9] [label=<" << "<table border='1' cellborder='0' cellspacing='0'>";

			result << "<tr><td bgcolor='gray22' port='header'><font color='gray81'>block ";
			
			#ifdef DEBUG
				result << block->id;
			#else
				result << block;
			#endif

			result << " (";
			
			for(size_t i = 0; i < block->var_count; ++i)
				result << (BitSetWrapper<MemoryPool>::get(block->in, i) ? "1" : "0");
			
			result << ")(";
			
			for(size_t i = 0; i < block->var_count; ++i)
				result << (BitSetWrapper<MemoryPool>::get(block->out, i) ? "1" : "0");
			
			result << ")</font></td></tr>";
			
			for(auto i = block->opcodes.begin(); i; ++i)
				result << "<tr><td align='left' bgcolor='gray95' port='C" << *i << "'><font color='gray52'>" << loc++ << ":</font> <font color='gray22'>" << printer.opcode(*i) << "</font></td></tr>";

			result << "</table>" << ">] [shape=plaintext];\n";

			if(block->next_block)
				result << print_link(block, block->next_block);
				
			if(block->branch_block)
				result << print_link(block, block->branch_block, false);
			
			return result.str();
		}

		void DotPrinter::print_block(Block *block, std::string filename)
		{
			std::fstream file(filename, std::ios_base::out);

			file << "digraph bytecode { \n";

			size_t loc = 0;
			
			for(auto i = block->basic_blocks.begin(); i; ++i)
			{
				file << print_basic_block(block, *i, loc) << "\n";
			}

			file << "\n}";
		}
	};
};
