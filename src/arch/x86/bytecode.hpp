#pragma once
#include "x86.hpp"
#include "../../codegen/bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct ByteCodeGenerator::Gen7<SuperOp, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>:
			public Gen
		{
			static SuperOp *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
			{
				Tree::Variable *module_var = lock(bcg.create_var(), Arch::Register::DX);
				Tree::Variable *name_var = lock(bcg.create_var(), Arch::Register::CX);
				
				bcg.gen<MoveOp>(module_var, std::forward<Arg3>(arg3));
				bcg.gen<MoveOp>(name_var, std::forward<Arg4>(arg4));

				return bcg.append(new (bcg.memory_pool) SuperOp(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), module_var, name_var, std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7)));
			}
		};
	};
};

