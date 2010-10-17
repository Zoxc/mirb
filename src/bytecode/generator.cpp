#include "generator.hpp"
#include "../tree/nodes.hpp"
#include "../../runtime/classes.hpp"
#include "../../runtime/classes/symbol.hpp"
#include "../../runtime/classes/fixnum.hpp"
#include "../../runtime/support.hpp"

#include "../compiler.hpp"

namespace Mirb
{
	void (ByteCodeGenerator::*ByteCodeGenerator::jump_table[SimpleNode::Types])(Node *basic_node, struct variable *var) = {
		0, // None
		&ByteCodeGenerator::convert_string,
		&ByteCodeGenerator::convert_interpolated_string,
		0, // InterpolatedPair
		&ByteCodeGenerator::convert_integer,
		&ByteCodeGenerator::convert_variable,
		&ByteCodeGenerator::convert_unary_op,
		&ByteCodeGenerator::convert_binary_op,
		&ByteCodeGenerator::convert_assignment,
		&ByteCodeGenerator::convert_self,
		&ByteCodeGenerator::convert_nil,
		&ByteCodeGenerator::convert_true,
		&ByteCodeGenerator::convert_false,
		&ByteCodeGenerator::convert_array,
		0, // Block
		0, // Invoke
		&ByteCodeGenerator::convert_call,
		&ByteCodeGenerator::convert_super,
		&ByteCodeGenerator::convert_break_handler,
		&ByteCodeGenerator::convert_if,
		&ByteCodeGenerator::convert_group,
		0, // Void
		&ByteCodeGenerator::convert_return,
		&ByteCodeGenerator::convert_next,
		&ByteCodeGenerator::convert_break,
		&ByteCodeGenerator::convert_redo,
		&ByteCodeGenerator::convert_class,
		&ByteCodeGenerator::convert_module,
		&ByteCodeGenerator::convert_method,
		0, // Rescue
		&ByteCodeGenerator::convert_handler
	};
	
	void ByteCodeGenerator::convert_string(Node *basic_node, struct variable *var)
	{
		if (var)
		{
			auto node = (StringNode *)basic_node;
			
			block_push(block, B_STRING, (rt_value)var, (rt_value)node->string, 0);
		}
	}
	
	void ByteCodeGenerator::convert_interpolated_string(Node *basic_node, struct variable *var)
	{
		auto node = (InterpolatedStringNode *)basic_node;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		size_t parameters = 0;
		
		for(auto i = node->pairs.begin(); i; i++)
		{
			if(strlen((const char *)i().string))
			{
				parameters++;
				
				block_push(block, B_STRING, (rt_value)temp, (rt_value)i().string, 0);
				block_push(block, B_PUSH, (rt_value)temp, 0, 0);
			}
			
			parameters++;
			
			to_bytecode(i().group, temp);
			block_push(block, B_PUSH, (rt_value)temp, 0, 0);
		}
	
		if(strlen((const char *)node->tail))
		{
			parameters++;
			
			block_push(block, B_STRING, (rt_value)temp, (rt_value)node->tail, 0);
			block_push(block, B_PUSH, (rt_value)temp, 0, 0);
		}
		
		struct opcode *args = block_push(block, B_ARGS, parameters, 0, 0);
		
		block_push(block, B_INTERPOLATE, (rt_value)var, 0, 0);
		
		block_push(block, B_ARGS_POP, (rt_value)args, 2, 0);
	}
	
	void ByteCodeGenerator::convert_integer(Node *basic_node, struct variable *var)
	{
		if (var)
		{
			auto node = (IntegerNode *)basic_node;
			
			block_push(block, B_MOV_IMM, (rt_value)var, RT_INT2FIX(node->value), 0);
		}
	}
	
	void ByteCodeGenerator::convert_variable(Node *basic_node, struct variable *var)
	{
		if(!var)
			return;
		
		auto node = (VariableNode *)basic_node;
		
		switch(node->variable_type)
		{
			case VariableNode::Temporary:
			case VariableNode::Local:
			{
				block_push(block, B_MOV, (rt_value)var, (rt_value)node->var, 0);
				return;
			}
			
			case VariableNode::Instance:
			{
				block->self_ref++;
				block_push(block, B_GET_IVAR, (rt_value)var, (rt_value)node->ivar.name, 0);
				return;
			}
			
			case VariableNode::Constant:
			{
				to_bytecode(node->constant.left, var);
				block_push(block, B_GET_CONST, (rt_value)var, (rt_value)var, (rt_value)node->constant.name);
				return;	
			}	
			
			default:
				RT_ASSERT(0);
		}
	}
	
	void ByteCodeGenerator::convert_unary_op(Node *basic_node, struct variable *var)
	{
		auto node = (UnaryOpNode *)basic_node;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		to_bytecode(node->value, temp);
		
		CallArgsInfo info = unary_call_args(0);
		
		block_push(block, B_CALL, (rt_value)temp, rt_symbol_from_cstr(Lexeme::names[node->op].c_str()), 0);
		
		call_args_seal(info);
		
		if (var)
			block_push(block, B_STORE, (rt_value)var, 0, 0);
	}
	
	void ByteCodeGenerator::convert_binary_op(Node *basic_node, struct variable *var)
	{
		auto node = (BinaryOpNode *)basic_node;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		to_bytecode(node->left, temp);
		
		CallArgsInfo info = binary_call_args(node->right, 0);
		
		block_push(block, B_CALL, (rt_value)temp, rt_symbol_from_cstr(Lexeme::names[node->op].c_str()), 0);
		
		call_args_seal(info);
		
		if (var)
			block_push(block, B_STORE, (rt_value)var, 0, 0);
	}
	
	void ByteCodeGenerator::convert_assignment(Node *basic_node, struct variable *var)
	{
		auto node = (AssignmentNode *)basic_node;
		
		auto variable = (VariableNode *)node->left;
		
		switch(variable->variable_type)
		{
			case VariableNode::Temporary:
			case VariableNode::Local:
			{
				to_bytecode(node->right, variable->var);
				
				if (var)
					block_push(block, B_MOV, (rt_value)var, (rt_value)variable->var, 0);
				
				return;
			}
			
			case VariableNode::Instance:
			{
				struct variable *temp = var ? var : block_get_var(block);
				
				to_bytecode(node->right, temp);
					
				block->self_ref++;
				block_push(block, B_SET_IVAR, (rt_value)variable->ivar.name, (rt_value)temp, 0);
				return;
			}
			
			case VariableNode::Constant:
			{
				struct variable *value = var ? var : block_get_var(block);
				struct variable *self = block_get_var(block);
				
				to_bytecode(variable->constant.left, self);
				
				to_bytecode(node->right, value);
				
				block_push(block, B_SET_CONST, (rt_value)self, (rt_value)variable->constant.name, (rt_value)value);
				return;	
			}	
			
			default:
				RT_ASSERT(0);
		}
	}
	
	void ByteCodeGenerator::convert_self(Node *basic_node, struct variable *var)
	{
		if(var)
		{
			block->self_ref++;
			block_push(block, B_SELF, (rt_value)var, 0, 0);
		}
	}
	
	void ByteCodeGenerator::convert_nil(Node *basic_node, struct variable *var)
	{
		if (var)
			block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
	}
	
	void ByteCodeGenerator::convert_true(Node *basic_node, struct variable *var)
	{
		if (var)
			block_push(block, B_MOV_IMM, (rt_value)var, RT_TRUE, 0);
	}
	
	void ByteCodeGenerator::convert_false(Node *basic_node, struct variable *var)
	{
		if (var)
			block_push(block, B_MOV_IMM, (rt_value)var, RT_FALSE, 0);
	}
	
	void ByteCodeGenerator::convert_array(Node *basic_node, struct variable *var)
	{
		auto node = (ArrayNode *)basic_node;
		
		size_t entries = 0;
		
		for(auto i = node->entries.begin(); i; i++)
		{
			to_bytecode(*i, var);
			
			block_push(block, B_PUSH, (rt_value)var, 0, 0);
			
			entries++;
		}
		
		struct opcode *args = block_push(block, B_ARGS, (rt_value)entries, 0, 0);
		
		block_push(block, B_ARRAY, (rt_value)var, 0, 0);
		
		block_push(block, B_ARGS_POP, (rt_value)args, 2, 0);
	}
	
	void ByteCodeGenerator::convert_call(Node *basic_node, struct variable *var)
	{
		auto node = (CallNode *)basic_node;
		
		call(node->object, node->method, node->arguments, node->block ? &node->block->scope : 0, var);
	}
	
	void ByteCodeGenerator::convert_super(Node *basic_node, struct variable *var)
	{
		auto node = (SuperNode *)basic_node;
		
		block->self_ref++;
		
		if(node->pass_args)
		{
			struct variable *closure = block->owner->block_parameter;
			
			/*
			 * Push arguments
			 */
			
			for(size_t i = 0; i < block->owner->parameters.size; i++)
			{
				block_push(block, B_PUSH, (rt_value)block->owner->parameters.array[i], 0, 0);
			}
			
			struct opcode *args = block_push(block, B_CALL_ARGS, (rt_value)block->owner->parameters.size, 0, 0);
			
			block_push(block, B_SUPER, (rt_value)closure, 0, 0);
			
			block_push(block, B_CALL_ARGS_POP, (rt_value)args, 0, 0);
			
			block_arg_seal(closure);
		}
		else
		{
			CallArgsInfo info = call_args(node->arguments, node->block ? &node->block->scope : 0, var);
			
			block_push(block, B_SUPER, (rt_value)info.closure, 0, 0);
			
			call_args_seal(info);
		}
		
		if (var)
			block_push(block, B_STORE, (rt_value)var, 0, 0);
	}
	
	void ByteCodeGenerator::convert_break_handler(Node *basic_node, struct variable *var)
	{
		auto node = (BreakHandlerNode *)basic_node;
		
		struct block *child = node->block;
		
		to_bytecode(node->code, var);
		
		struct opcode *label = 0;
		
		if(var)
		{
			/*
			 * Skip the break handler
			 */
			label = block_get_label(block);
			block_push(block, B_JMP, (rt_value)label, 0, 0);
		}
		
		/*
		 * Output target label
		 */
		block->data->break_targets[child->break_id] = (void *)block_emmit_label(block, block_get_flush_label(block));
		
		if(var)
		{
			block_push(block, B_STORE, (rt_value)var, 0, 0);
			block_emmit_label(block, label);
		}
	}
	
	void ByteCodeGenerator::convert_if(Node *basic_node, struct variable *var)
	{
		auto node = (IfNode *)basic_node;
		
		struct variable *temp = var ? var : block_get_var(block);
		struct opcode *label_else = block_get_label(block);
		
		to_bytecode(node->left, temp);
		
		block_push(block, B_TEST, (rt_value)temp, 0, 0);
		block_push(block, node->inverted ? B_JMPT : B_JMPF, (rt_value)label_else, 0, 0);
		
		struct opcode *label_end = block_get_label(block);
		
		if(var)
		{
			struct variable *result_left = block_get_var(block);
			struct variable *result_right = block_get_var(block);

			to_bytecode(node->middle, result_left);
			block_push(block, B_MOV, (rt_value)var, (rt_value)result_left, 0);
			block_push(block, B_JMP, (rt_value)label_end, 0, 0);

			block_emmit_label(block, label_else);

			to_bytecode(node->right, result_right);
			block_push(block, B_MOV, (rt_value)var, (rt_value)result_right, 0);

			block_emmit_label(block, label_end);
		}
		else
		{
			to_bytecode(node->middle, 0);
			block_push(block, B_JMP, (rt_value)label_end, 0, 0);

			block_emmit_label(block, label_else);

			to_bytecode(node->right, 0);

			block_emmit_label(block, label_end);
		}
	}
	
	void ByteCodeGenerator::convert_group(Node *basic_node, struct variable *var)
	{
		auto node = (GroupNode *)basic_node;
		
		if(node->statements.empty())
		{
			if (var)
				block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
				
			return;
		}
		
		for(auto i = node->statements.begin(); i; i++)
		{
			to_bytecode(*i, i().entry.next ? 0 : var, true);
		}
	}
	
	void ByteCodeGenerator::convert_return(Node *basic_node, struct variable *var)
	{
		auto node = (ReturnNode *)basic_node;
		
		if(!check_void_node(node))
			return;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		to_bytecode(node->value, temp);
		
		if(block->type == S_CLOSURE)
		{
			// TODO: only raise if this is a proc, not lambda
			
			//rt_value label = block_get_label(block);

			//block_push(block, B_TEST_PROC, 0, 0, 0);
			//block_push(block, B_JMPNE, label, 0, 0);
			block_push(block, B_RAISE_RETURN, (rt_value)temp, (rt_value)block->owner->data, 0);

			//block_emmit_label(block, label);
		}
		else if(has_ensure_block(block))
			block_push(block, B_RAISE_RETURN, (rt_value)temp, (rt_value)block->data, 0);
		else
			block_push(block, B_RETURN, (rt_value)temp, 0, 0);
	}
	
	void ByteCodeGenerator::convert_break(Node *basic_node, struct variable *var)
	{
		auto node = (BreakNode *)basic_node;
		
		if(!check_void_node(node))
			return;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		to_bytecode(node->value, temp);
		
		block_push(block, B_RAISE_BREAK, (rt_value)temp, (rt_value)block->parent->data, block->break_id);
	}
	
	void ByteCodeGenerator::convert_next(Node *basic_node, struct variable *var)
	{
		auto node = (NextNode *)basic_node;
		
		if(!check_void_node(node))
			return;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		to_bytecode(node->value, temp);
		
		block_push(block, B_RETURN, (rt_value)temp, 0, 0);
	}
	
	void ByteCodeGenerator::convert_redo(Node *basic_node, struct variable *var)
	{
		block_push(block, B_REDO, 0, 0, 0);
	}
	
	void ByteCodeGenerator::convert_class(Node *basic_node, struct variable *var)
	{
		auto node = (ClassNode *)basic_node;
		
		block->self_ref++;
		
		if(node->super)
		{
			struct variable *temp = var ? var : block_get_var(block);
			
			to_bytecode(node->super, temp);
			
			block_push(block, B_CLASS, (rt_value)node->name, (rt_value)temp, (rt_value)to_bytecode(node->scope));
		}
		else
			block_push(block, B_CLASS, (rt_value)node->name, 0, (rt_value)to_bytecode(node->scope));
		
		if (var)
			block_push(block, B_STORE, (rt_value)var, 0, 0);
	}
	
	void ByteCodeGenerator::convert_module(Node *basic_node, struct variable *var)
	{
		auto node = (ModuleNode *)basic_node;
		
		block->self_ref++;
		
		block_push(block, B_MODULE, (rt_value)node->name, (rt_value)to_bytecode(node->scope), 0);
		
		if (var)
			block_push(block, B_STORE, (rt_value)var, 0, 0);
	}
	
	void ByteCodeGenerator::convert_method(Node *basic_node, struct variable *var)
	{
		auto node = (MethodNode *)basic_node;
		
		block->self_ref++;
		
		block_push(block, B_METHOD, (rt_value)node->name, (rt_value)to_bytecode(node->scope), 0);

		if (var)
			block_push(block, B_MOV_IMM, (rt_value)var, RT_NIL, 0);
	}
	
	void ByteCodeGenerator::convert_handler(Node *basic_node, struct variable *var)
	{
		auto node = (HandlerNode *)basic_node;
		
		block->require_exceptions = true; // duh
		
		/*
		 * Allocate and setup the new exception block
		 */
		struct exception_block *exception_block = (struct exception_block *)malloc(sizeof(struct exception_block));
		size_t index = block->exception_blocks.size;
		size_t old_index = block->current_exception_block_id;
		
		exception_block->parent_index = old_index;
		exception_block->parent = block->current_exception_block;
		
		/*
		 * Check for ensure node
		 */
		if(node->ensure_group)
			exception_block->ensure_label = block_get_flush_label(block);
		else
			exception_block->ensure_label = 0;
		
		vec_init(rt_exception_handlers, &exception_block->handlers);

		vec_push(rt_exception_blocks, &block->exception_blocks, exception_block);
		
		/*
		 * Use the new exception block
		 */
		block->current_exception_block = exception_block;
		block->current_exception_block_id = index;
		block_push(block, B_HANDLER, index, 0, 0);
		
		/*
		 * Output the regular code
		 */
		exception_block->block_label = block_emmit_label(block, block_get_flush_label(block));
		
		to_bytecode(node->code, var);
		
		if(!node->rescues.empty())
		{
			/*
			 * Skip the rescue block
			 */
			struct opcode *ok_label = block_get_label(block);
			block_push(block, B_JMP, (rt_value)ok_label, 0, 0);
			
			/*
			 * Output rescue nodes. TODO: Check for duplicate nodes
			 */
			for(auto i = node->rescues.begin(); i; ++i)
			{
				struct runtime_exception_handler *handler = (struct runtime_exception_handler *)malloc(sizeof(struct runtime_exception_handler));
				handler->common.type = E_RUNTIME_EXCEPTION;
				handler->rescue_label = block_emmit_label(block, block_get_flush_label(block));
				vec_push(rt_exception_handlers, &exception_block->handlers, (struct exception_handler *)handler);
				
				to_bytecode(i().group, var);
				
				block_push(block, B_JMP, (rt_value)ok_label, 0, 0);
			}
			
			block_emmit_label(block, ok_label);
		}
		
		/*
		 * Restore the old exception frame
		 */
		block->current_exception_block_id = old_index;
		block->current_exception_block = exception_block->parent;
		block_push(block, B_HANDLER, old_index, 0, 0);
		
		/*
		 * Check for ensure node
		 */
		if(node->ensure_group)
		{
			exception_block->ensure_label = block_emmit_label(block, (struct opcode *)exception_block->ensure_label);
			
			/*
			 * Output ensure node
			 */
			to_bytecode(node->ensure_group, 0);
			
			block_push(block, B_ENSURE_RET, index, 0, 0);
		}
	}
	
	ByteCodeGenerator::ByteCodeGenerator(Compiler &compiler) : compiler(compiler) 
	{
	}
	
	void ByteCodeGenerator::to_bytecode(Node *node, struct variable *var, bool allow_void)
	{
		this->allow_void = allow_void;
		
		(this->*jump_table[node->type()])(node, var);
	}
	
	struct block *ByteCodeGenerator::to_bytecode(Scope &scope)
	{
		struct block *prev_block = block;
		
		block = scope.block;
		
		struct variable *result = block_get_var(block);
		
		for(size_t i = 0; i < block->zsupers.size; i++)
		{
			block_require_args(block->zsupers.array[i], block);
		}
		
		block->epilog = block_get_label(block);
		
		if(block->break_targets)
		{
			block->require_exceptions = true;
			block->data->break_targets = (void **)malloc(block->break_targets * sizeof(void *));
		}
		else
			block->data->break_targets = 0;
		
		to_bytecode(scope.group, result);
		
		struct opcode *last = block->opcodes.array[block->opcodes.size - 1];
		
		if(last->type == B_RETURN && last->left == (rt_value)result)
			last->type = B_LOAD;
		else
			block_push(block, B_LOAD, (rt_value)result, 0, 0);
		
		block_emmit_label(block, block->epilog);
		
		/*
		 * Assign an unique id to each variable of type V_HEAP and V_LOCAL
		 */
		
		for(hash_iter_t i = hash_begin(block->variables); i != hash_end(block->variables); i++)
		{
			if(hash_exist(block->variables, i))
			{
				struct variable *var = hash_value(block->variables, i);

				switch(var->type)
				{
					case V_HEAP:
					case V_LOCAL:
						var->index = block->var_count[var->type]++;
						break;

					default:
						break;
				}
			}
		}
		
		#ifdef DEBUG
			printf(";\n; block %x\n;\n", (rt_value)block);

			block_print(block);

			printf("\n\n");
		#endif
		
		block = prev_block;
		
		return scope.block;
	}
	
	bool ByteCodeGenerator::has_ensure_block(struct block *block)
	{
		struct exception_block *exception_block = block->current_exception_block;

		while(exception_block)
		{
			if(exception_block->ensure_label != (void *)-1)
				return true;

			exception_block = exception_block->parent;
		}

		return false;
	}

	bool ByteCodeGenerator::check_void_node(VoidNode *node)
	{
		if(!allow_void)
		{
			compiler.report(*node->range, "Unexpected void value in expression");
			return false;
		}
		else
			return true;
	}
	
	void ByteCodeGenerator::call(Node *self, Symbol *name, NodeList &arguments, Scope *scope, struct variable *var)
	{
		struct variable *self_var = var ? var : block_get_var(block);
		
		to_bytecode(self, self_var);
		
		CallArgsInfo info = call_args(arguments, scope, 0);
		
		block_push(block, B_CALL, (rt_value)self_var, (rt_value)name, (rt_value)info.closure);
		
		call_args_seal(info);
		
		if (var)
			block_push(block, B_STORE, (rt_value)var, 0, 0);
	}
	
	struct variable *ByteCodeGenerator::block_arg(Scope *scope)
	{
		struct variable* closure = 0;
		
		if(scope)
		{
			block->self_ref++;
			
			closure = block_get_var(block);
			struct block *block_attach = to_bytecode(*scope);
			
			for(size_t i = 0; i < block_attach->scopes.size; i++)
				block_push(block, B_PUSH_SCOPE, (rt_value)block_attach->scopes.array[i], 0, 0);
			
			struct opcode *args = block_push(block, B_ARGS, block_attach->scopes.size, 0, 0);
			
			block_push(block, B_CLOSURE, (rt_value)closure, (rt_value)block_attach, 0);
			
			block_push(block, B_ARGS_POP, (rt_value)args, 5, 0);
		}
		
		return closure;
	}

	void ByteCodeGenerator::block_arg_seal(struct variable *closure)
	{
		if(closure)
			block_push(block, B_SEAL, (rt_value)closure, 0, 0);
	}

	ByteCodeGenerator::CallArgsInfo ByteCodeGenerator::call_args(NodeList &arguments, Scope *scope, struct variable *var)
	{
		CallArgsInfo result;
		
		size_t parameters = 0;
		
		struct variable *temp = var ? var : block_get_var(block);
		
		for(auto i = arguments.begin(); i; i++)
		{
			parameters++;
			
			to_bytecode(*i, temp);
			
			block_push(block, B_PUSH, (rt_value)temp, 0, 0);
		}
		
		result.closure = block_arg(scope);
		result.args = block_push(block, B_CALL_ARGS, parameters, (rt_value)result.closure, 0);
		
		return result;
	}
	
	void ByteCodeGenerator::call_args_seal(CallArgsInfo & info)
	{
		block_push(block, B_CALL_ARGS_POP, (rt_value)info.args, 0, 0);
		
		block_arg_seal(info.closure);
	}
	
	ByteCodeGenerator::CallArgsInfo ByteCodeGenerator::unary_call_args(struct variable *var)
	{
		NodeList list;
		
		return call_args(list, 0, var);
	}
	
	ByteCodeGenerator::CallArgsInfo ByteCodeGenerator::binary_call_args(Node *arg, struct variable *var)
	{
		NodeList list;
		
		list.append(arg);
		
		return call_args(list, 0, var);
	}
};
