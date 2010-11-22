#pragma once
#include "x86.hpp"
#include "../../codegen/bytecode.hpp"

namespace Mirb
{
	namespace CodeGen
	{
		template<> struct ByteCodeGenerator::Gen0<PrologueOp>:
			public Gen
		{
			static PrologueOp *gen(ByteCodeGenerator &bcg)
			{
				if(bcg.block->heap_array_var)
					bcg.gen<MoveOp>(bcg.block->heap_array_var, lock(bcg.create_var(), Arch::Register::AX));
				
				if(bcg.scope->super_name_var)
					bcg.write_variable(bcg.scope->super_module_var, lock(bcg.create_var(), Arch::Register::CX));

				if(bcg.scope->super_module_var)
					bcg.write_variable(bcg.scope->super_module_var, lock(bcg.create_var(), Arch::Register::DX));

				if(bcg.block->heap_var)
					bcg.gen<CreateHeapOp>(bcg.block->heap_var);
				
				Tree::Variable *block_parameter = bcg.block->scope->block_parameter;

				if(block_parameter)
				{
					if(block_parameter->type == Tree::Variable::Heap)
					{
						Tree::Variable *value = bcg.create_var();

						bcg.gen<LoadArgOp>(value, LoadArgOp::Block);

						bcg.write_variable(block_parameter, value);
					}
					else
						bcg.gen<LoadArgOp>(block_parameter, LoadArgOp::Block);
				}

				if(!bcg.scope->parameters.empty())
				{
					Tree::Variable *argv = bcg.create_var();
					bcg.gen<LoadArgOp>(argv, LoadArgOp::ArgValues);

					size_t index = 0;
					for(auto i = bcg.scope->parameters.begin(); i != bcg.scope->parameters.end(); ++i, ++index)
					{
						if(i().type == Tree::Variable::Heap)
						{
							Tree::Variable *value = bcg.create_var();

							bcg.gen<LookupOp>(value, argv, index);

							bcg.write_variable(*i, value);
						}
						else
							bcg.gen<LookupOp>(*i, argv, index);
					}
				}
				
				if(bcg.block->self_var)
					bcg.gen<LoadArgOp>(bcg.block->self_var, LoadArgOp::Object);

				return 0;
			}
		};

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

