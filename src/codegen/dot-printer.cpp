#include <fstream>
#include "dot-printer.hpp"
#include "printer.hpp"
#include "opcodes.hpp"
#include "block.hpp"

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

			if(!regular)
				result << "[color=coral4]";

			result << ";\n";

			return result.str();
		}

		std::string DotPrinter::print_basic_block(BasicBlock *block)
		{
			ByteCodePrinter printer;
			
			std::stringstream result;

			result << "B" << block << " [fontname=Courier New] [fontsize=9] [label=<" << "<table border='1' cellborder='0' cellspacing='0'>";

			result << "<tr><td bgcolor='gray22' port='header'><font color='gray81'>block " << block->id << "</font></td></tr>";
			
			for(auto i = block->opcodes.begin(); i; ++i)
				result << "<tr><td align='left' bgcolor='gray95' port='C" << *i << "'><font color='gray22'>" << printer.opcode(*i) << "</font></td></tr>";

			result << "</table>" << ">] [shape=plaintext];\n";

			if(block->next)
				result << print_link(block, block->next);
				
			if(block->branch)
				result << print_link(block,block->branch, false);
			
			return result.str();
		}

		void DotPrinter::print_block(Block *block, std::string filename)
		{
			std::fstream file(filename, std::ios_base::out);

			file << "digraph bytecode { \n";
			
			for(auto i = block->basic_blocks.begin(); i; ++i)
			{
				file << print_basic_block(*i) << "\n";
			}

			file << "\n}";
		}
	};
};
