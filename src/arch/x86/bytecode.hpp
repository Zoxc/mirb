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
				Tree::Variable *name_var = arg4;
				Tree::Variable *module_var = arg5;
				Tree::Variable *self_var = arg6;
				Tree::Variable *argc = arg7;
				Tree::Variable *argv = arg8;
				
				Tree::Variable **setup_argv = new (bcg.memory_pool) Tree::Variable *[3];
				size_t setup_argc = 0;

				if(heap_array_var)
				{
					setup_argv[setup_argc++] = heap_array_var;
					bcg.gen<MoveOp>(heap_array_var, lock(bcg.create_var(), Arch::Register::BX));
				}
				
				if(argc)
				{
					setup_argv[setup_argc++] = argc;
					bcg.write_variable(argc, lock(bcg.create_var(), Arch::Register::SI));
				}
				
				if(argv)
				{
					setup_argv[setup_argc++] = argv;
					bcg.gen<MoveOp>(argv, lock(bcg.create_var(), Arch::Register::DI));
				}

				bcg.gen<SetupVarsOp>(setup_argv, setup_argc);

				if(heap_var)
					bcg.gen<CreateHeapOp>(heap_var);
				
				if(self_var)
					bcg.gen<LoadArgOp>(self_var, Arch::StackArg::Self);
				
				if(name_var)
					bcg.gen<LoadArgOp>(name_var, Arch::StackArg::Name);
				
				if(module_var)
					bcg.gen<LoadArgOp>(module_var, Arch::StackArg::Module);
				
				if(block_parameter)
					bcg.gen<LoadArgOp>(block_parameter, Arch::StackArg::Block);
				
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

