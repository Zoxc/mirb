#pragma once

#include "../../compiler/bytecode.hpp"
#include "../../compiler/block.hpp"
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
				void convert_break_handler(Tree::Node *basic_node, Tree::Variable *var);
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
				
				struct CallArgsInfo
				{
					Tree::Variable *closure;
					struct opcode *args;
				};
				
				static void (ByteCodeGenerator::*jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, Tree::Variable *var);
				
				struct block *block;
				Block *_block;
				
				Tree::Scope *scope;
				
				bool has_ensure_block(struct block *block);
						
				void to_bytecode(Tree::Node *node, Tree::Variable *var)
				{
					(this->*jump_table[node->type()])(node, var);
				}
				
				Tree::Variable *reuse(Tree::Variable *var)
				{
					return var ? var : create_var();
				}
				
				Tree::Variable *create_var();
				
				Label *create_label();
				
				template<class T, typename ...Args> T *gen(Args&&... params)
				{
					T *result = new (memory_pool) T(std::forward<Args>(params)...);
					_block->opcodes.append(result);
					return result;
				}
				
				void gen(Label *label)
				{
					_block->opcodes.append(label);
				}
				
				void call(Tree::Node *self, Symbol *name, Tree::NodeList &arguments, Tree::Scope *scope, Tree::Variable *var);
				Tree::Variable *block_arg(Tree::Scope *scope);
				void block_arg_seal(Tree::Variable *closure);
				CallArgsInfo call_args(Tree::NodeList &arguments, Tree::Scope *scope, Tree::Variable *var);
				void call_args_seal(CallArgsInfo &info);
				CallArgsInfo unary_call_args(Tree::Variable *var);
				CallArgsInfo binary_call_args(Tree::Node *arg, Tree::Variable *var);
				
				MemoryPool &memory_pool;
			public:
				ByteCodeGenerator(MemoryPool &memory_pool);
				
				Block *to_bytecode(Tree::Scope *scope);
		};
	};
};
