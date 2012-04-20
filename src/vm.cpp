#include "vm.hpp"
#include "runtime.hpp"

namespace Mirb
{

#define Op(name) case CodeGen::Opcode::name: { auto &op = *(CodeGen::name##Op *)ip; ip += sizeof(CodeGen::name##Op);
#define EndOp break; }

	value_t evaluate_block(Block *code, value_t self, Symbol *name, value_t module, value_t block, size_t argc, value_t argv[])
	{
		const char *ip = code->opcodes;

		value_t *vars = new value_t[code->var_words];

		for(size_t i = 0; i < code->var_words; ++i)
			vars[i] = value_nil;

		while(true)
			switch(*ip)
			{
				Op(Move)
					vars[op.dst] = vars[op.src];
				EndOp
					
				Op(Load)
					vars[op.var] = op.imm;
				EndOp

				Op(Branch)
					ip = code->opcodes + op.pos;
				EndOp
					
				Op(BranchUnless)
					if(!Value::test(vars[op.var]))
						ip = code->opcodes + op.pos;
				EndOp

				Op(BranchIf)
					if(Value::test(vars[op.var]))
						ip = code->opcodes + op.pos;
				EndOp

				Op(Return)
					goto end;
				EndOp
			}

	end:
		value_t result = vars[code->return_var];

		delete[] vars;

		return result;
	}
};
