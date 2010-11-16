#pragma once
#include "../../codegen/bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<> ClosureOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1, Tree::Variable *arg2, Block *arg3, size_t arg4);
		template<> CallOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1, Tree::Variable *arg2, Symbol *arg3, size_t arg4, Tree::Variable *arg5, size_t arg6);
		template<> ReturnOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1);
	};
};

