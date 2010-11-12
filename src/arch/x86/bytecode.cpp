#include "x86.hpp"
#include "bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<> ClosureOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1, Tree::Variable *arg2, Block *arg3, size_t arg4)
		{
			Tree::Variable *self = lock(create_var(), Arch::Register::ecx);

			gen<MoveOp>(self, arg2);
			
			Tree::Variable *result_var = lock(create_var(), Arch::Register::eax);

			ClosureOp *result = new (memory_pool) ClosureOp(result_var, self, arg3, arg4);
			
			append(result);

			gen<MoveOp>(arg1, result_var);
			
			return result;
		}
	};
};

