#pragma once
#include "../../codegen/bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<> ClosureOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1, Tree::Variable *arg2, Block *arg3, size_t arg4);
		template<> ReturnOp *ByteCodeGenerator::gen<>(Tree::Variable *arg1);
	};
};

