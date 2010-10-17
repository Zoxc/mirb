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
			void convert_string(Node *basic_node, struct variable *var);
			void convert_interpolated_string(Node *basic_node, struct variable *var);
			void convert_integer(Node *basic_node, struct variable *var);
			void convert_variable(Node *basic_node, struct variable *var);
			void convert_unary_op(Node *basic_node, struct variable *var);
			void convert_binary_op(Node *basic_node, struct variable *var);
			void convert_assignment(Node *basic_node, struct variable *var);
			void convert_self(Node *basic_node, struct variable *var);
			void convert_nil(Node *basic_node, struct variable *var);
			void convert_true(Node *basic_node, struct variable *var);
			void convert_false(Node *basic_node, struct variable *var);
			void convert_array(Node *basic_node, struct variable *var);
			void convert_call(Node *basic_node, struct variable *var);
			void convert_super(Node *basic_node, struct variable *var);
			void convert_break_handler(Node *basic_node, struct variable *var);
			void convert_if(Node *basic_node, struct variable *var);
			void convert_group(Node *basic_node, struct variable *var);
			void convert_return(Node *basic_node, struct variable *var);
			void convert_break(Node *basic_node, struct variable *var);
			void convert_next(Node *basic_node, struct variable *var);
			void convert_redo(Node *basic_node, struct variable *var);
			void convert_class(Node *basic_node, struct variable *var);
			void convert_module(Node *basic_node, struct variable *var);
			void convert_method(Node *basic_node, struct variable *var);
			void convert_handler(Node *basic_node, struct variable *var);
			
			struct CallArgsInfo
			{
				struct variable *closure;
				struct opcode *args;
			};
			
			static void (ByteCodeGenerator::*jump_table[SimpleNode::Types])(Node *basic_node, struct variable *var);
			
			struct block *block;
			bool allow_void;
			
			bool has_ensure_block(struct block *block);
			bool check_void_node(VoidNode *node);
			void to_bytecode(Node *basic_node, struct variable *var, bool allow_void = false);
			void call(Node *self, Symbol *name, NodeList &arguments, Scope *scope, struct variable *var);
			struct variable *block_arg(Scope *scope);
			void block_arg_seal(struct variable *closure);
			CallArgsInfo call_args(NodeList &arguments, Scope *scope, struct variable *var);
			void call_args_seal(CallArgsInfo &info);
			CallArgsInfo unary_call_args(struct variable *var);
			CallArgsInfo binary_call_args(Node *arg, struct variable *var);
			
			Compiler &compiler;
		public:
			ByteCodeGenerator(Compiler &compiler);
			
			struct block *to_bytecode(Scope &scope);
	};
};
