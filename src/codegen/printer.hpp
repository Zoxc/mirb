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
		class BasicBlock;
		
		class ByteCodePrinter
		{
			private:
				ByteCodeGenerator *bcg;
				const char *data;

				std::string imm(Symbol *imm);
				std::string label(const char *opcode);
				std::string label(Label *label);
				Label *get_label(const char *opcode);
				std::string raw(size_t imm);
				std::string print_block(Mirb::Block *block);
			public:
				ByteCodePrinter(ByteCodeGenerator *bcg) : bcg(bcg), highlight(0) {}

				Tree::Variable *highlight;
				
				std::string var(var_t var);
				std::string var_name(Tree::NamedVariable *var);
				std::string opcode(const char *&opcode);
				std::string print();
		};
	};
};
