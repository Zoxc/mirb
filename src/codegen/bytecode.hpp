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
				
				Label *create_label();
				
				// TODO: Fix this silly C++ workaround
				Tree::Variable *null_var()
				{
					return 0;
				}
				
				template<class T, typename ...Args> T *gen(Args&&... params)
				{
					T *result = new (memory_pool) T(std::forward<Args>(params)...);
					
					block->opcodes.append(result);
					
					return result;
				}
				
				Label *gen(Label *label)
				{
					block->opcodes.append(label);
					
					return label;
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
