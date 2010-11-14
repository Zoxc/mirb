#include "x86.hpp"
#include "bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<> ClosureOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1, Tree::Variable *arg2, Block *arg3, size_t arg4)
		{
			Tree::Variable *self = lock(create_var(), Arch::Register::CX);

			gen<MoveOp>(self, arg2);
			
			Tree::Variable *result_var = lock(create_var(), Arch::Register::AX);

			ClosureOp *result = new (memory_pool) ClosureOp(result_var, self, arg3, arg4);
			
			append(result);

			gen<MoveOp>(arg1, result_var);
			
			return result;
		}

		template<> ReturnOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1)
		{
			Tree::Variable *result_var = lock(create_var(), Arch::Register::AX);
			
			gen<MoveOp>(result_var, arg1);
			
			ReturnOp *result = new (memory_pool) ReturnOp(result_var);
			
			append(result);

			return result;
		}
	};
};

