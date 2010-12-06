#pragma once
#include "x86.hpp"
#include "../../codegen/bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> struct ByteCodeGenerator::Gen8<PrologueOp, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>:
			public Gen
		{
			static PrologueOp *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
			{
				Tree::Variable *heap_array_var = arg1;
				Tree::Variable *heap_var = arg2;
				Tree::Variable *block_parameter = arg3;
				Tree::Variable *method_name_var = arg4;
				Tree::Variable *method_module_var = arg5;
				Tree::Variable *self_var = arg6;
				Tree::Variable *argc = arg7;
				Tree::Variable *argv = arg8;

				if(heap_array_var)
					bcg.gen<MoveOp>(heap_array_var, lock(bcg.create_var(), Arch::Register::AX));
				
				if(block_parameter)
					bcg.write_variable(block_parameter, lock(bcg.create_var(), Arch::Register::CX));

				if(method_module_var)
					bcg.gen<MoveOp>(method_module_var, lock(bcg.create_var(), Arch::Register::DX));

				if(heap_var)
					bcg.gen<CreateHeapOp>(heap_var);
				
				if(method_name_var)
					bcg.gen<LoadArgOp>(method_name_var, Arch::StackArg::MethodName);
				
				if(self_var)
					bcg.gen<LoadArgOp>(self_var, Arch::StackArg::Self);
				
				if(argc)
					bcg.gen<LoadArgOp>(argc, Arch::StackArg::Count);
				
				if(argv)
					bcg.gen<LoadArgOp>(argv, Arch::StackArg::Values);
				
				return 0;
			}
		};
		
		template<typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct ByteCodeGenerator::Gen7<SuperOp, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>:
			public Gen
		{
			static SuperOp *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
			{
				Tree::Variable *module_var = lock(bcg.create_var(), Arch::Register::DX);
				
				bcg.gen<MoveOp>(module_var, std::forward<Arg3>(arg3));

				return bcg.append(new (bcg.memory_pool) SuperOp(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), module_var, std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7)));
			}
		};
	};
};

