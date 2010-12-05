#pragma once

#include "../tree/nodes.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	};
	
	class MemoryPool;
	
	namespace CodeGen
	{
		class ByteCodeGenerator
		{
			private:
				void convert_string(Tree::Node *basic_node, Tree::Variable *var);
				void convert_interpolated_string(Tree::Node *basic_node, Tree::Variable *var);
				void convert_integer(Tree::Node *basic_node, Tree::Variable *var);
				void convert_variable(Tree::Node *basic_node, Tree::Variable *var);
				void convert_ivar(Tree::Node *basic_node, Tree::Variable *var);
				void convert_constant(Tree::Node *basic_node, Tree::Variable *var);
				void convert_unary_op(Tree::Node *basic_node, Tree::Variable *var);
				void convert_boolean_not(Tree::Node *basic_node, Tree::Variable *var);
				void convert_binary_op(Tree::Node *basic_node, Tree::Variable *var);
				void convert_boolean_op(Tree::Node *basic_node, Tree::Variable *var);
				void convert_assignment(Tree::Node *basic_node, Tree::Variable *var);
				void convert_self(Tree::Node *basic_node, Tree::Variable *var);
				void convert_nil(Tree::Node *basic_node, Tree::Variable *var);
				void convert_true(Tree::Node *basic_node, Tree::Variable *var);
				void convert_false(Tree::Node *basic_node, Tree::Variable *var);
				void convert_array(Tree::Node *basic_node, Tree::Variable *var);
				void convert_call(Tree::Node *basic_node, Tree::Variable *var);
				void convert_super(Tree::Node *basic_node, Tree::Variable *var);
				void convert_if(Tree::Node *basic_node, Tree::Variable *var);
				void convert_group(Tree::Node *basic_node, Tree::Variable *var);
				void convert_return(Tree::Node *basic_node, Tree::Variable *var);
				void convert_break(Tree::Node *basic_node, Tree::Variable *var);
				void convert_next(Tree::Node *basic_node, Tree::Variable *var);
				void convert_redo(Tree::Node *basic_node, Tree::Variable *var);
				void convert_class(Tree::Node *basic_node, Tree::Variable *var);
				void convert_module(Tree::Node *basic_node, Tree::Variable *var);
				void convert_method(Tree::Node *basic_node, Tree::Variable *var);
				void convert_handler(Tree::Node *basic_node, Tree::Variable *var);

				static void (ByteCodeGenerator::*jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, Tree::Variable *var);
				
				Block *block;

				BasicBlock *body; // The point after the prolog of the block.
				
				Tree::Scope *scope;
				
				bool has_ensure_block(Block *block);
				
				Mirb::Block *compile(Tree::Scope *scope);
				Mirb::Block *defer(Tree::Scope *scope);
				
				void to_bytecode(Tree::Node *node, Tree::Variable *var)
				{
					(this->*jump_table[node->type()])(node, var);
				}
				
				Tree::Variable *reuse(Tree::Variable *var)
				{
					// TODO: Make sure no code stores results in the returned variable
					return create_var();
				}
				
				Tree::Variable *self_var();
				
				BranchIfOp *gen_if(BasicBlock *ltrue, Tree::Variable *var)
				{
					BranchIfOp *result = gen<BranchIfOp>(ltrue, var);
					
					basic->branch(result->label);

					return result;
				}
				
				BranchUnlessOp *gen_unless(BasicBlock *lfalse, Tree::Variable *var)
				{
					BranchUnlessOp *result = gen<BranchUnlessOp>(lfalse, var);
					
					basic->branch(result->label);

					return result;
				}
				
				BranchOp *gen_branch(BasicBlock *block)
				{
					BranchOp *result = gen<BranchOp>(block);
					
					basic->next(block);
					
					return result;
				}
				
				BasicBlock *list(BasicBlock *block)
				{
					this->block->basic_blocks.append(block);

					return block;
				}
				
				void branch(BasicBlock *block)
				{
					gen_branch(block);
					
					//TODO: Eliminate code generation from this point. The block will never be reached
					gen(create_block());
				}
				
				Tree::Variable *block_arg(Tree::Scope *scope);
				Tree::Variable *call_args(Tree::NodeList &arguments, size_t &param_count, Tree::Scope *scope, Tree::Variable *var);
				
			public:
				ByteCodeGenerator(MemoryPool &memory_pool);

				MemoryPool &memory_pool;
				
				BasicBlock *split(BasicBlock *block)
				{
					basic->next(block);
					gen(block);

					return block;
				}
				
				BasicBlock *gen(BasicBlock *block)
				{
					basic = block;
					list(block);

					return block;
				}
				
				// TODO: Fix this silly C++ workaround
				Tree::Variable *null_var()
				{
					return 0;
				}
				
				template<class T> T *append(T *op)
				{
					basic->opcodes.append(op);

					return op;
				}

				struct Gen
				{
					static Tree::Variable *lock(Tree::Variable *var, size_t reg);
				};

				template<class T> struct Gen0:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg)
					{
						return bcg.append(new (bcg.memory_pool) T);
					}
				};
				
				template<class T, typename Arg1> struct Gen1:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1)));
					}
				};
				
				template<class T, typename Arg1, typename Arg2> struct Gen2:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)));
					}
				};
				
				template<class T, typename Arg1, typename Arg2, typename Arg3> struct Gen3:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3)));
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct Gen4:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4)));
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct Gen5:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5)));
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> struct Gen6:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6)));
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct Gen7:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7)));
					}
				};
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> struct Gen8:
					public Gen
				{
					static T *gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
					{
						return bcg.append(new (bcg.memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7), std::forward<Arg8>(arg8)));
					}
				};

				template<class T> T *gen()
				{
					return Gen0<T>::gen(*this);
				}

				template<class T, typename Arg1> T *gen(Arg1 arg1)
				{
					return Gen1<T, Arg1>::gen(*this, std::forward<Arg1>(arg1));
				}

				template<class T, typename Arg1, typename Arg2> T *gen(Arg1&& arg1, Arg2&& arg2)
				{
					return Gen2<T, Arg1, Arg2>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
				{
					return Gen3<T, Arg1, Arg2, Arg3>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
				{
					return Gen4<T, Arg1, Arg2, Arg3, Arg4>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
				{
					return Gen5<T, Arg1, Arg2, Arg3, Arg4, Arg5>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
				{
					return Gen6<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6));
				}
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
				{
					return Gen7<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7));
				}
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
				{
					return Gen8<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7), std::forward<Arg8>(arg8));
				}
				
				BasicBlock *basic;

				Tree::Variable *create_var();
				BasicBlock *create_block();
				
				Tree::Variable *read_variable(Tree::Variable *var);
				void write_variable(Tree::Variable *var, Tree::Variable *value);
				
				Block *to_bytecode(Tree::Scope *scope);
				Block *create();
		};
	};
};

#include "../arch/bytecode.hpp"
