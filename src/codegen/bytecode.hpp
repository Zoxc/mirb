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

				BasicBlock *basic;
				
				Tree::Scope *scope;
				
				bool has_ensure_block(Block *block);
				
				void to_bytecode(Tree::Node *node, Tree::Variable *var)
				{
					(this->*jump_table[node->type()])(node, var);
				}
				
				Tree::Variable *reuse(Tree::Variable *var)
				{
					return var ? var : create_var();
				}
				
				Tree::Variable *create_var();
				
				Tree::Variable *self_var();
				
				BasicBlock *create_block();
				
				// TODO: Fix this silly C++ workaround
				Tree::Variable *null_var()
				{
					return 0;
				}
				
				
				#ifdef _MSC_VER
					template<class T> T *gen()
					{
						T *result = new (memory_pool) T;
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1> T *gen(Arg1&& arg1)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1));
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1, typename Arg2> T *gen(Arg1&& arg1, Arg2&& arg2)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1, typename Arg2, typename Arg3> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3));
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4));
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5));
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6));
						
						basic->opcodes.append(result);
						
						return result;
					}

					template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> T *gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
					{
						T *result = new (memory_pool) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7));
						
						basic->opcodes.append(result);
						
						return result;
					}
				#else
					template<class T, typename ...Args> T *gen(Args&&... params)
					{
						T *result = new (memory_pool) T(std::forward<Args>(params)...);
					
						basic->opcodes.append(result);
					
						return result;
					}
				#endif

				BranchIfOp *gen_if(BasicBlock *ltrue, Tree::Variable *var)
				{
					BranchIfOp *result = gen<BranchIfOp>(ltrue, var);
					
					basic->branch = result->ltrue;

					return result;
				}
				
				BranchUnlessOp *gen_unless(BasicBlock *lfalse, Tree::Variable *var)
				{
					BranchUnlessOp *result = gen<BranchUnlessOp>(lfalse, var);
					
					basic->branch = result->lfalse;

					return result;
				}
				
				BasicBlock *list(BasicBlock *block)
				{
					this->block->basic_blocks.append(block);

					return block;
				}
				
				BasicBlock *gen(BasicBlock *block)
				{
					basic = block;
					list(block);

					return block;
				}
				
				BasicBlock *split(BasicBlock *block)
				{
					basic->next = block;
					gen(block);

					return block;
				}
				
				void branch(BasicBlock *block)
				{
					gen<BranchOp>(block);
					basic->next = block;
					basic = create_block();
					list(basic); // TODO: Don't list or even generate anything on this since it's useless
					basic->next = 0; // Nothing may set this. Remove when the list call is removed.
				}

				Tree::Variable *block_arg(Tree::Scope *scope);
				Tree::Variable *call_args(Tree::NodeList &arguments, size_t &param_count, Tree::Scope *scope, Tree::Variable *var);
				
				MemoryPool &memory_pool;
			public:
				ByteCodeGenerator(MemoryPool &memory_pool);
				
				Block *to_bytecode(Tree::Scope *scope);
		};
	};
};
