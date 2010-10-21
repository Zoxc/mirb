#pragma once

#include "../../compiler/bytecode.hpp"
#include "../../compiler/block.hpp"
#include "../tree/nodes.hpp"

namespace Mirb
{
	class Compiler;
	
	class ByteCodeGenerator
	{
		private:
			void convert_string(Tree::Node *basic_node, struct variable *var);
			void convert_interpolated_string(Tree::Node *basic_node, struct variable *var);
			void convert_integer(Tree::Node *basic_node, struct variable *var);
			void convert_variable(Tree::Node *basic_node, struct variable *var);
			void convert_unary_op(Tree::Node *basic_node, struct variable *var);
			void convert_boolean_not(Tree::Node *basic_node, struct variable *var);
			void convert_binary_op(Tree::Node *basic_node, struct variable *var);
			void convert_boolean_op(Tree::Node *basic_node, struct variable *var);
			void convert_assignment(Tree::Node *basic_node, struct variable *var);
			void convert_self(Tree::Node *basic_node, struct variable *var);
			void convert_nil(Tree::Node *basic_node, struct variable *var);
			void convert_true(Tree::Node *basic_node, struct variable *var);
			void convert_false(Tree::Node *basic_node, struct variable *var);
			void convert_array(Tree::Node *basic_node, struct variable *var);
			void convert_call(Tree::Node *basic_node, struct variable *var);
			void convert_super(Tree::Node *basic_node, struct variable *var);
			void convert_break_handler(Tree::Node *basic_node, struct variable *var);
			void convert_if(Tree::Node *basic_node, struct variable *var);
			void convert_group(Tree::Node *basic_node, struct variable *var);
			void convert_return(Tree::Node *basic_node, struct variable *var);
			void convert_break(Tree::Node *basic_node, struct variable *var);
			void convert_next(Tree::Node *basic_node, struct variable *var);
			void convert_redo(Tree::Node *basic_node, struct variable *var);
			void convert_class(Tree::Node *basic_node, struct variable *var);
			void convert_module(Tree::Node *basic_node, struct variable *var);
			void convert_method(Tree::Node *basic_node, struct variable *var);
			void convert_handler(Tree::Node *basic_node, struct variable *var);
			
			struct CallArgsInfo
			{
				struct variable *closure;
				struct opcode *args;
			};
			
			static void (ByteCodeGenerator::*jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, struct variable *var);
			
			struct block *block;
			bool allow_void;
			
			bool has_ensure_block(struct block *block);
			bool check_void_node(Tree::VoidNode *node);
			void to_bytecode(Tree::Node *basic_node, struct variable *var, bool allow_void = false);
			void call(Tree::Node *self, Symbol *name, Tree::NodeList &arguments, Tree::Scope *scope, struct variable *var);
			struct variable *block_arg(Tree::Scope *scope);
			void block_arg_seal(struct variable *closure);
			CallArgsInfo call_args(Tree::NodeList &arguments, Tree::Scope *scope, struct variable *var);
			void call_args_seal(CallArgsInfo &info);
			CallArgsInfo unary_call_args(struct variable *var);
			CallArgsInfo binary_call_args(Tree::Node *arg, struct variable *var);
			
			Compiler &compiler;
		public:
			ByteCodeGenerator(Compiler &compiler);
			
			struct block *to_bytecode(Tree::Scope &scope);
	};
};
