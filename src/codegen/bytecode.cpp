#include "bytecode.hpp"
#include "../arch/bytecode.hpp"
#include "../tree/nodes.hpp"
#include "../../runtime/classes.hpp"
#include "../../runtime/classes/symbol.hpp"
#include "../../runtime/classes/fixnum.hpp"
#include "../../runtime/support.hpp"

#include "../tree/tree.hpp"
#include "../compiler.hpp"
#include "../block.hpp"

#ifdef DEBUG
	#include "printer.hpp"
	#include "dot-printer.hpp"
#endif

namespace Mirb
{
	namespace CodeGen
	{
		void (ByteCodeGenerator::*ByteCodeGenerator::jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, Tree::Variable *var) = {
			0, // None
			&ByteCodeGenerator::convert_string,
			&ByteCodeGenerator::convert_interpolated_string,
			0, // InterpolatedPair
			&ByteCodeGenerator::convert_integer,
			&ByteCodeGenerator::convert_variable,
			&ByteCodeGenerator::convert_ivar,
			&ByteCodeGenerator::convert_constant,
			&ByteCodeGenerator::convert_unary_op,
			&ByteCodeGenerator::convert_boolean_not,
			&ByteCodeGenerator::convert_binary_op,
			&ByteCodeGenerator::convert_boolean_op,
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
		
		void ByteCodeGenerator::convert_string(Tree::Node *basic_node, Tree::Variable *var)
		{
			if (var)
			{
				auto node = (Tree::StringNode *)basic_node;
				
				gen<StringOp>(var, node->string);
			}
		}
		
		void ByteCodeGenerator::convert_interpolated_string(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::InterpolatedStringNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			size_t parameters = 0;
			
			for(auto i = node->pairs.begin(); i != node->pairs.end(); ++i)
			{
				if(strlen((const char *)i().string))
				{
					parameters++;
					
					gen<StringOp>(temp, i().string);
					gen<PushOp>(temp);
				}
				
				parameters++;
				
				to_bytecode(i().group, temp);
				gen<PushOp>(temp);
			}
		
			if(strlen((const char *)node->tail))
			{
				parameters++;
				
				gen<StringOp>(temp, node->tail);
				gen<PushOp>(temp);
			}
			
			gen<InterpolateOp>(var, parameters);
		}
		
		void ByteCodeGenerator::convert_integer(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(var)
			{
				auto node = (Tree::IntegerNode *)basic_node;
				
				gen<LoadOp>(var, RT_INT2FIX(node->value));
			}
		}
		
		void ByteCodeGenerator::convert_variable(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(!var)
				return;

			auto node = (Tree::VariableNode *)basic_node;

			if(node->var->type == Tree::Variable::Heap)
			{
				Tree::Variable *heap;

				if(node->var->owner != scope)
				{
					heap = reuse(var);

					gen<GetHeapOp>(heap, block->heap_array_var, scope->referenced_scopes.index_of(node->var->owner));
				}
				else
					heap = block->heap_var;

				gen<GetHeapVarOp>(var, heap, node->var);
			}
			else
				gen<MoveOp>(var, node->var);
		}
		
		void ByteCodeGenerator::convert_ivar(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(!var)
				return;
			
			auto node = (Tree::IVarNode *)basic_node;
			
			gen<GetIVarOp>(var, self_var(), node->name);
		}
		
		void ByteCodeGenerator::convert_constant(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::ConstantNode *)basic_node;
			
			if(node->obj)
				to_bytecode(node->obj, var);
			else if(var)
				gen<LoadOp>(var, rt_Object);
			
			if(!var)
				return;
				
			gen<GetConstOp>(var, var, node->name);
		}
		
		void ByteCodeGenerator::convert_unary_op(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::UnaryOpNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<CallOp>(var, temp, (Symbol *)rt_symbol_from_cstr(Lexeme::names[node->op].c_str()), (size_t)0, null_var(), Tree::InvokeNode::no_break_id);
		}
		
		void ByteCodeGenerator::convert_boolean_not(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::BooleanNotNode *)basic_node;
			
			if(var)
			{
				Tree::Variable *temp = reuse(var);
				
				to_bytecode(node->value, temp);
				
				BasicBlock *label_true = create_block();
				BasicBlock *label_end = create_block();
				BasicBlock *label_false = create_block();
				
				gen_if(label_true, temp);

				split(label_false);
				gen<LoadOp>(var, RT_TRUE);
				gen<BranchOp>(label_end);
				label_false->next(label_end);

				gen(label_true);
				gen<LoadOp>(var, RT_FALSE);
				label_true->next(label_end);
				
				gen(label_end);
			}
			else
				to_bytecode(node->value, 0);
		}
		
		void ByteCodeGenerator::convert_binary_op(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::BinaryOpNode *)basic_node;
			
			if(node->op == Lexeme::LOGICAL_AND || node->op == Lexeme::LOGICAL_OR)
			{
				convert_boolean_op(basic_node, var);
				return;
			}
			
			Tree::Variable *left = reuse(var);
			
			to_bytecode(node->left, left);
			
			Tree::Variable *right = create_var();
			
			to_bytecode(node->right, right);
			
			gen<PushOp>(right);
			
			gen<CallOp>(var, left, (Symbol *)rt_symbol_from_cstr(Lexeme::names[node->op].c_str()), (size_t)1, null_var(), Tree::InvokeNode::no_break_id);
		}
		
		void ByteCodeGenerator::convert_boolean_op(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::BinaryOpNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			to_bytecode(node->left, temp);
			
			BasicBlock *label_end = create_block();
			BasicBlock *body = create_block();
			
			if(node->op == Lexeme::KW_OR || node->op == Lexeme::LOGICAL_OR)
				gen_if(label_end, temp);
			else
				gen_unless(label_end, temp);

			split(body);
			
			to_bytecode(node->right, var);
			
			gen(label_end);
		}
		
		void ByteCodeGenerator::convert_assignment(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::AssignmentNode *)basic_node;
			
			switch(node->left->type())
			{
				case Tree::SimpleNode::Variable:
				{
					auto variable = (Tree::VariableNode *)node->left;

					if(variable->var->type == Tree::Variable::Heap)
					{
						Tree::Variable *temp = reuse(var);

						to_bytecode(node->right, temp);

						Tree::Variable *heap;

						if(variable->var->owner != scope)
						{
							heap = create_var();

							gen<GetHeapOp>(heap, block->heap_array_var, scope->referenced_scopes.index_of(variable->var->owner));
						}
						else
							heap = block->heap_var;

						gen<SetHeapVarOp>(heap, variable->var, temp);

						if(var)
							gen<MoveOp>(var, temp);
					}
					else
					{
						to_bytecode(node->right, variable->var);
					
						if(var)
							gen<MoveOp>(var, variable->var);
					}
					
					return;
				}
				
				case Tree::SimpleNode::IVar:
				{
					auto variable = (Tree::IVarNode *)node->left;
					
					Tree::Variable *temp = reuse(var);
					
					to_bytecode(node->right, temp);
						
					gen<SetIVarOp>(self_var(), variable->name, temp);
					
					return;
				}
				
				case Tree::SimpleNode::Constant:
				{
					auto variable = (Tree::ConstantNode *)node->left;
					
					Tree::Variable *value = reuse(var);
					Tree::Variable *obj = create_var();
					
					if(variable->obj)
						to_bytecode(variable->obj, obj);
					else
						gen<LoadOp>(obj, rt_Object);
					
					to_bytecode(node->right, value);
					
					gen<SetConstOp>(obj, variable->name, value);
					
					return;	
				}	
				
				default:
					debug_fail("Unknown left hand expression");
			}
		}
		
		void ByteCodeGenerator::convert_self(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(var)
				gen<MoveOp>(var, self_var());
		}
		
		void ByteCodeGenerator::convert_nil(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(var)
				gen<LoadOp>(var, RT_NIL);
		}
		
		void ByteCodeGenerator::convert_true(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(var)
				gen<LoadOp>(var, RT_TRUE);
		}
		
		void ByteCodeGenerator::convert_false(Tree::Node *basic_node, Tree::Variable *var)
		{
			if(var)
				gen<LoadOp>(var, RT_FALSE);
		}
		
		void ByteCodeGenerator::convert_array(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::ArrayNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			if(!var)
			{
				for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
					to_bytecode(*i, 0);
				
				return;
			}
			
			size_t entries = 0;
			
			for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
			{
				to_bytecode(*i, temp);
				
				gen<PushOp>(temp);
				
				entries++;
			}
			
			gen<ArrayOp>(var, entries);	
		}
		
		void ByteCodeGenerator::convert_call(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::CallNode *)basic_node;
			
			Tree::Variable *obj = reuse(var);
			
			to_bytecode(node->object, obj);
			
			size_t param_count;
			
			Tree::Variable *closure = call_args(node->arguments, param_count, node->block ? node->block->scope : 0, 0);
			
			gen<CallOp>(var, obj, node->method, param_count, closure, node->break_id);
			
			if(node->break_id != Tree::InvokeNode::no_break_id)
				block->final->break_targets[node->break_id] = (void *)gen<BreakTargetOp>();
		}
		
		void ByteCodeGenerator::convert_super(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::SuperNode *)basic_node;
			
			if(node->pass_args)
			{
				Tree::Variable *closure = scope->owner->block_parameter;
				
				// push arguments
				
				size_t param_count = 0;
				
				for(auto i = scope->owner->parameters.begin(); i != scope->owner->parameters.end(); ++i, ++param_count)
				{
					gen<PushOp>(*i);
				}
				
				gen<SuperOp>(var, self_var(), scope->super_module_var, scope->super_name_var, param_count, closure, node->break_id);
			}
			else
			{
				size_t param_count;
				
				Tree::Variable *closure = call_args(node->arguments, param_count, scope, 0);
				
				gen<SuperOp>(var, self_var(), scope->super_module_var, scope->super_name_var, param_count, closure, node->break_id);
			}
			
			if(node->break_id != Tree::InvokeNode::no_break_id)
				block->final->break_targets[node->break_id] = (void *)gen<BreakTargetOp>();
		}
		
		void ByteCodeGenerator::convert_if(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::IfNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);

			BasicBlock *label_else = create_block();
			BasicBlock *label_end = create_block();
			BasicBlock *body = create_block();
			
			body->next(label_end);
			label_else->next(label_end);

			to_bytecode(node->left, temp);
			
			if(node->inverted)
				gen_if(label_else, temp);
			else
				gen_unless(label_else, temp);
			
			split(body);
			
			if(var)
			{
				Tree::Variable *result_left = create_var();
				Tree::Variable *result_right = create_var();

				to_bytecode(node->middle, result_left);
				
				gen<MoveOp>(var, result_left);
				gen<BranchOp>(label_end);

				gen(label_else);

				to_bytecode(node->right, result_right);
				
				gen<MoveOp>(var, result_right);
				gen(label_end);
			}
			else
			{
				to_bytecode(node->middle, 0);
				
				gen<BranchOp>(label_end);

				gen(label_else);

				to_bytecode(node->right, 0);

				gen(label_end);
			}
		}
		
		void ByteCodeGenerator::convert_group(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::GroupNode *)basic_node;
			
			if(node->statements.empty())
			{
				if (var)
					gen<LoadOp>(var, RT_NIL);
				
				return;
			}
			
			for(auto i = node->statements.begin(); i != node->statements.end(); ++i)
			{
				to_bytecode(*i, i().entry.next ? 0 : var);
			}
		}
		
		void ByteCodeGenerator::convert_return(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::ReturnNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			if(scope->type == Tree::Scope::Closure)
			{
				// TODO: only raise if this is a proc, not lambda
				
				//rt_value label = create_label();

				//block_push(block, B_TEST_PROC, 0, 0, 0);
				//block_push(block, B_JMPNE, label, 0, 0);
				gen<UnwindReturnOp>(temp, scope->owner->block->final);

				//block_emmit_label(block, label);
			}
			else if(has_ensure_block(block))
				gen<UnwindReturnOp>(temp, block->final);
			else
			{
				gen<MoveOp>(block->return_var, temp);
				branch(block->epilog);
			}
		}
		
		void ByteCodeGenerator::convert_break(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::BreakNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<UnwindBreakOp>(temp, scope->parent->block->final, scope->break_id);
		}
		
		void ByteCodeGenerator::convert_next(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::NextNode *)basic_node;
			
			Tree::Variable *temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<MoveOp>(block->return_var, temp);
			branch(block->epilog);
		}
		
		void ByteCodeGenerator::convert_redo(Tree::Node *basic_node, Tree::Variable *var)
		{
			branch(block->prolog);
		}
		
		void ByteCodeGenerator::convert_class(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::ClassNode *)basic_node;
			
			Tree::Variable *super;
			
			if(node->super)
			{
				super = reuse(var);
				
				to_bytecode(node->super, super);
			}
			else
				super = 0;
			
			gen<ClassOp>(var, self_var(), node->name, super, to_bytecode(node->scope));
		}
		
		void ByteCodeGenerator::convert_module(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::ModuleNode *)basic_node;
			
			gen<ModuleOp>(var, self_var(), node->name, to_bytecode(node->scope));
		}
		
		void ByteCodeGenerator::convert_method(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::MethodNode *)basic_node;
			
			gen<MethodOp>(self_var(), node->name, to_bytecode(node->scope));
			
			if(var)
				gen<LoadOp>(var, RT_NIL);
		}
		
		void ByteCodeGenerator::convert_handler(Tree::Node *basic_node, Tree::Variable *var)
		{
			auto node = (Tree::HandlerNode *)basic_node;
			
			scope->require_exceptions = true; // duh
			
			/*
			 * Allocate and setup the new exception block
			 */
			struct exception_block *exception_block = (struct exception_block *)malloc(sizeof(struct exception_block));
			size_t index = block->final->exception_blocks.size();
			size_t old_index = block->current_exception_block_id;
			
			exception_block->parent_index = old_index;
			exception_block->parent = block->current_exception_block;
			
			/*
			 * Check for ensure node
			 */
			if(node->ensure_group)
				exception_block->ensure_label = (void *)create_block();
			else
				exception_block->ensure_label = 0;
			
			vec_init(rt_exception_handlers, &exception_block->handlers);

			block->final->exception_blocks.push(exception_block);
			
			/*
			 * Use the new exception block
			 */
			block->current_exception_block = exception_block;
			block->current_exception_block_id = index;
			gen<HandlerOp>(index);
			
			/*
			 * Output the regular code
			 */
			BasicBlock *body = split(create_block());

			exception_block->block_label = (void *)body;
			
			to_bytecode(node->code, var);
			
			if(!node->rescues.empty())
			{
				/*
				 * Skip the rescue block
				 */
				BasicBlock *ok_label = create_block();
				gen<BranchOp>(ok_label);
				body->next(ok_label);
				
				/*
				 * Output rescue nodes. TODO: Check for duplicate nodes
				 */
				for(auto i = node->rescues.begin(); i != node->rescues.end(); ++i)
				{
					struct runtime_exception_handler *handler = (struct runtime_exception_handler *)malloc(sizeof(struct runtime_exception_handler));
					handler->common.type = E_RUNTIME_EXCEPTION;

					BasicBlock *handler_body = gen(create_block());

					handler->rescue_label = (void *)handler_body;
					vec_push(rt_exception_handlers, &exception_block->handlers, (struct exception_handler *)handler);
					
					to_bytecode(i().group, var);
					
					gen<BranchOp>(ok_label);
					handler_body->next(ok_label);
				}
				
				gen(ok_label);
			}
			
			/*
			 * Restore the old exception frame
			 */
			block->current_exception_block_id = old_index;
			block->current_exception_block = exception_block->parent;
			gen<HandlerOp>(old_index);
			
			/*
			 * Check for ensure node
			 */
			if(node->ensure_group)
			{
				split((BasicBlock *)exception_block->ensure_label);
				
				/*
				 * Output ensure node
				 */
				to_bytecode(node->ensure_group, 0);
				
				gen<UnwindOp>();
			}
		}
		
		ByteCodeGenerator::ByteCodeGenerator(MemoryPool &memory_pool) : memory_pool(memory_pool) 
		{
		}
		
		Block *ByteCodeGenerator::to_bytecode(Tree::Scope *scope)
		{
			Block *block = new (memory_pool) Block(memory_pool, scope);
			block->final = new (gc) Mirb::Block;
			
			scope->block = block;
			
			Tree::Scope *prev_scope = this->scope;
			Block *prev_block = this->block;
			BasicBlock *prev_basic = this->basic;
			
			this->scope = scope;
			this->block = block;
			
			block->return_var = create_var();

			if(scope->heap_vars)
				block->heap_var = create_var();

			if(scope->referenced_scopes.size() > 0)
				block->heap_array_var = create_var();
			
			for(auto i = scope->zsupers.begin(); i != scope->zsupers.end(); ++i)
			{
				scope->require_args(*i);
			}

			block->prolog = create_block();
			block->epilog = create_block();
			
			if(scope->break_targets)
			{
				scope->require_exceptions = true;
				block->final->break_targets = (void **)malloc(scope->break_targets * sizeof(void *));
			}
			else
				block->final->break_targets = 0;
			
			gen(block->prolog);

			to_bytecode(scope->group, block->return_var);
			
			split(block->epilog);

			gen<ReturnOp>(block->return_var);

			block->epilog->next_block = 0;

			block->analyse_liveness();
			block->allocate_registers();

			#ifdef DEBUG
				DotPrinter dot_printer;
				ByteCodePrinter printer(block);

				std::cout << printer.print();

				std::system("mkdir bytecode");
				
				dot_printer.print_block(block, "bytecode/bytecode.dot");

				std::stringstream path;
				
				path << "bytecode/block-" << block;
				
				std::system(("mkdir \"" + path.str() + "\"").c_str());
				std::system(("dot -Tpng bytecode/bytecode.dot -o " + path.str() + ".png").c_str());
				
				for(auto i = scope->variable_list.begin(); i != scope->variable_list.end(); ++i)
				{
					dot_printer.highlight = *i;

					dot_printer.print_block(block, "bytecode/bytecode.dot");
					
					std::stringstream result;

					result << "dot -Tpng bytecode/bytecode.dot -o " << path.str() << "/var" << i()->index << ".png";

					std::system(result.str().c_str());
				}
			#endif

			this->block = prev_block;
			this->basic = prev_basic;
			this->scope = prev_scope;
			
			return block;
		}
		
		bool ByteCodeGenerator::has_ensure_block(Block *block)
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
		
		BasicBlock *ByteCodeGenerator::create_block()
		{
			return new (memory_pool) BasicBlock(memory_pool, *block);
		}
		
		Tree::Variable *ByteCodeGenerator::self_var()
		{
			if(!block->self_var)
				block->self_var = create_var();
			
			return block->self_var;
		}
		
		Tree::Variable *ByteCodeGenerator::create_var()
		{
			auto var = new (memory_pool) Tree::Variable(Tree::Variable::Temporary);

			var->owner = scope;
			var->index = scope->variable_list.size();

			scope->variable_list.push(var);
			
			return var;
		}

		Tree::Variable *ByteCodeGenerator::block_arg(Tree::Scope *scope)
		{
			Tree::Variable *var = 0;
			
			if(scope)
			{
				var = create_var();
				Block *block_attach = to_bytecode(scope);
				
				size_t scopes = 0;

				for(auto i = scope->referenced_scopes.begin(); i != scope->referenced_scopes.end(); ++i, ++scopes)
				{
					if(*i == this->scope)
						gen<PushOp>(block->heap_var);
					else
					{
						gen<GetHeapOp>(var, block->heap_array_var, scope->referenced_scopes.index_of(i));
						gen<PushOp>(var);
					}
				}
				
				gen<ClosureOp, Tree::Variable *, Tree::Variable *, Block *, size_t>(var, self_var(), block_attach, scopes);
			}
			
			return var;
		}
		
		Tree::Variable *ByteCodeGenerator::call_args(Tree::NodeList &arguments, size_t &param_count, Tree::Scope *scope, Tree::Variable *var)
		{
			size_t parameters = 0;

			if(!arguments.empty())
			{
				Tree::Variable *temp = reuse(var);
			
				for(auto i = arguments.begin(); i != arguments.end(); ++i)
				{
					parameters++;
				
					to_bytecode(*i, temp);
				
					gen<PushOp>(temp);
				}
			}
			
			param_count = parameters;
			
			return block_arg(scope);
		}
	};
};
