#pragma once

#include "../tree/nodes.hpp"
#include "block.hpp"

namespace Mirb
{
	namespace Tree
	{
		class Scope;
	};
	
	namespace CodeGen
	{
		class ByteCodeGenerator;
		
		class VariableGroup
		{
			private:
				size_t address;
				ByteCodeGenerator *bcg;
				var_t var;
			public:
				VariableGroup(ByteCodeGenerator *bcg, size_t size);

				var_t operator[](size_t index);
				
				var_t use();

				size_t size;
		};

		class ByteCodeGenerator
		{
			private:
				void convert_string(Tree::Node *basic_node, var_t var);
				void convert_interpolated_string(Tree::Node *basic_node, var_t var);
				void convert_integer(Tree::Node *basic_node, var_t var);
				void convert_variable(Tree::Node *basic_node, var_t var);
				void convert_ivar(Tree::Node *basic_node, var_t var);
				void convert_constant(Tree::Node *basic_node, var_t var);
				void convert_unary_op(Tree::Node *basic_node, var_t var);
				void convert_boolean_not(Tree::Node *basic_node, var_t var);
				void convert_binary_op(Tree::Node *basic_node, var_t var);
				void convert_boolean_op(Tree::Node *basic_node, var_t var);
				void convert_assignment(Tree::Node *basic_node, var_t var);
				void convert_self(Tree::Node *basic_node, var_t var);
				void convert_nil(Tree::Node *basic_node, var_t var);
				void convert_true(Tree::Node *basic_node, var_t var);
				void convert_false(Tree::Node *basic_node, var_t var);
				void convert_array(Tree::Node *basic_node, var_t var);
				void convert_call(Tree::Node *basic_node, var_t var);
				void convert_super(Tree::Node *basic_node, var_t var);
				void convert_if(Tree::Node *basic_node, var_t var);
				void convert_group(Tree::Node *basic_node, var_t var);
				void convert_return(Tree::Node *basic_node, var_t var);
				void convert_break(Tree::Node *basic_node, var_t var);
				void convert_next(Tree::Node *basic_node, var_t var);
				void convert_redo(Tree::Node *basic_node, var_t var);
				void convert_class(Tree::Node *basic_node, var_t var);
				void convert_module(Tree::Node *basic_node, var_t var);
				void convert_method(Tree::Node *basic_node, var_t var);
				void convert_handler(Tree::Node *basic_node, var_t var);

				static void (ByteCodeGenerator::*jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, var_t var);
				
				BasicBlock *body; // The point after the prolog of the block.
				
				Tree::Scope *scope;
				
				bool has_ensure_block(Block *block);
				
				Mirb::Block *compile(Tree::Scope *scope);
				Mirb::Block *defer(Tree::Scope *scope);
				
				void to_bytecode(Tree::Node *node, var_t var)
				{
					(this->*jump_table[node->type()])(node, var);
				}
				
				var_t reuse(var_t var)
				{
					// TODO: Make sure no code stores results in the returned variable
					return create_var();
				}
				
				var_t ref(Tree::Variable *var)
				{
					if(var)
						return var->loc;
					else
						return no_var;
				}
				
				var_t self_var();

				void gen_if(BasicBlock *ltrue, var_t var)
				{
					basic->branches.push(BasicBlock::BranchInfo(gen<BranchIfOp>(var), ltrue));
				}
				
				void gen_unless(BasicBlock *lfalse, var_t var)
				{
					basic->branches.push(BasicBlock::BranchInfo(gen<BranchUnlessOp>(var), lfalse));
				}
				
				void location(Range *range)
				{
					basic->source_locs.push(BasicBlock::SourceInfo((size_t)basic->opcodes.tellp(), range));
				}
				
				void gen_branch(BasicBlock *block)
				{
					basic->branches.push(BasicBlock::BranchInfo(gen<BranchOp>(), block));
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
				
				var_t block_arg(Tree::Scope *scope, var_t break_dst);
				var_t call_args(Tree::CountedNodeList &arguments, Tree::Scope *scope, size_t &argc, var_t &argv, var_t break_dst);
				
			public:
				ByteCodeGenerator(MemoryPool memory_pool);

				MemoryPool memory_pool;
				Block *block;
				
				BasicBlock *gen(BasicBlock *block)
				{
					basic = block;
					list(block);

					return block;
				}
				
				bool is_var(var_t var)
				{
					return var != no_var;
				}
				
				template<class T> size_t append(T &op)
				{
					basic->opcodes.write((const char *)&op, sizeof(T));
					return (size_t)basic->opcodes.tellp() - sizeof(T);
				}

				struct Gen
				{
				};

				template<class T> struct Gen0:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg)
					{
						T op;
						return bcg.append(op);
					}
				};
				
				template<class T, typename Arg1> struct Gen1:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1)
					{
						T op(std::forward<Arg1>(arg1));
						return bcg.append(op);
					}
				};
				
				template<class T, typename Arg1, typename Arg2> struct Gen2:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));
						return bcg.append(op);
					}
				};
				
				template<class T, typename Arg1, typename Arg2, typename Arg3> struct Gen3:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3));
						return bcg.append(op);
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> struct Gen4:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4));
						return bcg.append(op);
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> struct Gen5:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5));
						return bcg.append(op);
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> struct Gen6:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6));
						return bcg.append(op);
					}
				};

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> struct Gen7:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7));
						return bcg.append(op);
					}
				};
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> struct Gen8:
					public Gen
				{
					static size_t gen(ByteCodeGenerator &bcg, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
					{
						T op(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7), std::forward<Arg8>(arg8));
						return bcg.append(op);
					}
				};

				template<class T> size_t gen()
				{
					return Gen0<T>::gen(*this);
				}

				template<class T, typename Arg1> size_t gen(Arg1 arg1)
				{
					return Gen1<T, Arg1>::gen(*this, std::forward<Arg1>(arg1));
				}

				template<class T, typename Arg1, typename Arg2> size_t gen(Arg1&& arg1, Arg2&& arg2)
				{
					return Gen2<T, Arg1, Arg2>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
				{
					return Gen3<T, Arg1, Arg2, Arg3>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
				{
					return Gen4<T, Arg1, Arg2, Arg3, Arg4>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
				{
					return Gen5<T, Arg1, Arg2, Arg3, Arg4, Arg5>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5));
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
				{
					return Gen6<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6));
				}
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
				{
					return Gen7<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7));
				}
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7, Arg8&& arg8)
				{
					return Gen8<T, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, Arg8>::gen(*this, std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7), std::forward<Arg8>(arg8));
				}
				
				BasicBlock *basic;

				var_t create_var();
				BasicBlock *create_block();
				
				var_t read_variable(Tree::Variable *var);
				void write_variable(Tree::Variable *var, var_t value);
				
				Block *to_bytecode(Tree::Scope *scope);
				Block *create();
		};
	};
};
