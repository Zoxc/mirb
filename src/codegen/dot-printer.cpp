#include "../common.hpp"
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

		std::string DotPrinter::print_basic_block(ByteCodePrinter &printer, Block *main_block, BasicBlock *block, size_t &loc)
		{
			std::stringstream result;

			result << "B" << block << " [fontname=Courier New] [fontsize=9] [label=<" << "<table border='1' cellborder='0' cellspacing='0'>";

			result << "<tr><td bgcolor='gray22' port='header'><font color='gray81'>block ";
			
			#ifdef DEBUG
				result << block->id;
			#else
				result << block;
			#endif

			result << "</font></td></tr>";
			
			for(auto i = block->opcodes.begin(); i != block->opcodes.end(); ++i)
			{
				result << "<tr><td align='left' bgcolor='";

				if(highlight && highlight->range.contains(loc))
					result << "#D9E2E8";
				else
					result << "gray95";

				result << "' port='C" << *i << "'><font color='gray52'>" << loc++ << ":</font> <font color='gray22'>" << printer.opcode(*i) << "</font></td></tr>";
			}

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

			ByteCodePrinter printer(block);

			printer.highlight = highlight;
			
			file << "digraph bytecode { \n";
			
			file << "vars [fontname=Courier New] [fontsize=9] [label=<" << "<table border='1' cellborder='0' cellspacing='0'>";

			file << "<tr><td bgcolor='gray22'><font color='gray81'>var</font></td><td bgcolor='gray22'><font color='gray81'>reg</font></td></tr>";

			for(auto i = 0; i < block->var_count; ++i)
			{
				file << "<tr><td align='left' bgcolor='gray95'><font color='gray52'>" << printer.var(i) << "</font></td><td align='left' bgcolor='grey90'><font color='gray30'>" << i << "</font></td></tr>";
			}

			file << "</table>" << ">] [shape=plaintext];\n";

			size_t loc = 0;
			
			for(auto i = block->basic_blocks.begin(); i != block->basic_blocks.end(); ++i)
			{
				file << print_basic_block(printer, block, *i, loc) << "\n";
			}

			file << "\n}";
		}
	};
};
