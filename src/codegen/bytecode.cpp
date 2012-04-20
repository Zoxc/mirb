#include "bytecode.hpp"
#include "../tree/nodes.hpp"
#include "../tree/tree.hpp"
#include "../compiler.hpp"
#include "../block.hpp"
#include "../classes/fixnum.hpp"

namespace Mirb
{
	const size_t CodeGen::no_var = -1;

	namespace CodeGen
	{
		// TODO: Fix - This code assumes the stack grows downwards.

		VariableGroup::VariableGroup(ByteCodeGenerator *bcg, size_t size) : bcg(bcg), size(size)
		{
			if(size)
			{
				var = bcg->block->var_count;
				bcg->block->var_count += size;
			}
		}

		var_t VariableGroup::operator[](size_t index)
		{
			if(size)
			{
				return var + index;
			}
			else
			{
				return no_var;
			}
		}
		
		var_t VariableGroup::use()
		{
			if(size)
			{
				// TODO:  return unused variables to the list

				return var;
			}
			else
			{
				return no_var;
			}
		}

		void (ByteCodeGenerator::*ByteCodeGenerator::jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, var_t var) = {
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
		
		void ByteCodeGenerator::convert_string(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
			{
				auto node = (Tree::StringNode *)basic_node;
				
				gen<StringOp>(var, node->string);
			}
		}
		
		void ByteCodeGenerator::convert_interpolated_string(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::InterpolatedStringNode *)basic_node;
			
			size_t param_count = 0;

			for(auto i = node->pairs.begin(); i != node->pairs.end(); ++i)
			{
				if(strlen((const char *)i().string))
					param_count++;
				
				param_count++;
			}
			
			if(strlen((const char *)node->tail))
				param_count++;
			
			VariableGroup group(this, param_count);

			size_t param = 0;
			
			for(auto i = node->pairs.begin(); i != node->pairs.end(); ++i)
			{
				if(strlen((const char *)i().string))
					gen<StringOp>(group[param++], i().string);
				
				to_bytecode(i().group, group[param++]);
			}
			
			if(strlen((const char *)node->tail))
				gen<StringOp>(group[param++], node->tail);
			
			gen<InterpolateOp>(var, group.size, group.use());
		}
		
		void ByteCodeGenerator::convert_integer(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
			{
				auto node = (Tree::IntegerNode *)basic_node;
				
				gen<LoadOp>(var, Fixnum::from_int(node->value));
			}
		}

		var_t ByteCodeGenerator::read_variable(Tree::Variable *var)
		{
			if(var->type == Tree::Variable::Heap)
			{
				var_t heap;

				if(var->owner != scope)
				{
					heap = create_var();

					gen<LookupOp>(heap, block->heap_array_var, scope->referenced_scopes.index_of(var->owner));
				}
				else
					heap = block->heap_var;

				var_t result = create_var();

				gen<GetHeapVarOp>(result, heap, var->loc);

				return result;
			}
			
			return ref(var);
		}

		void ByteCodeGenerator::convert_variable(Tree::Node *basic_node, var_t var)
		{
			if(!is_var(var))
				return;

			auto node = (Tree::VariableNode *)basic_node;

			gen<MoveOp>(var, read_variable(node->var));
		}
		
		void ByteCodeGenerator::convert_ivar(Tree::Node *basic_node, var_t var)
		{
			if(!is_var(var))
				return;
			
			auto node = (Tree::IVarNode *)basic_node;
			
			gen<GetIVarOp>(var, self_var(), node->name);
		}
		
		void ByteCodeGenerator::convert_constant(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ConstantNode *)basic_node;
			
			if(node->obj)
				to_bytecode(node->obj, var);
			else if(is_var(var))
				gen<LoadOp>(var, Object::class_ref);
			
			if(!is_var(var))
				return;
				
			gen<GetConstOp>(var, var, node->name);
		}
		
		void ByteCodeGenerator::convert_unary_op(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::UnaryOpNode *)basic_node;
			
			var_t temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<CallOp>(var, temp, Symbol::from_string(Lexeme::names[node->op].c_str()), no_var, (Mirb::Block *)0, (size_t)0, no_var);
		}
		
		void ByteCodeGenerator::convert_boolean_not(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BooleanNotNode *)basic_node;
			
			if(is_var(var))
			{
				var_t temp = reuse(var);
				
				to_bytecode(node->value, temp);
				
				BasicBlock *label_true = create_block();
				BasicBlock *label_end = create_block();
				BasicBlock *label_false = create_block();
				
				gen_if(label_true, temp);

				split(label_false);
				gen<LoadOp>(var, value_true);
				gen_branch(label_end);

				gen(label_true);
				gen<LoadOp>(var, value_false);
				label_true->next(label_end);
				
				gen(label_end);
			}
			else
				to_bytecode(node->value, 0);
		}
		
		void ByteCodeGenerator::convert_binary_op(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BinaryOpNode *)basic_node;
			
			if(node->op == Lexeme::LOGICAL_AND || node->op == Lexeme::LOGICAL_OR)
			{
				convert_boolean_op(basic_node, var);
				return;
			}
			
			var_t left = reuse(var);
			
			to_bytecode(node->left, left);

			VariableGroup group(this, 1);
			
			to_bytecode(node->right, group[0]);
			
			gen<CallOp>(var, left, Symbol::from_string(Lexeme::names[node->op].c_str()), no_var, (Mirb::Block *)0, group.size, group.use());
		}
		
		void ByteCodeGenerator::convert_boolean_op(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BinaryOpNode *)basic_node;
			
			var_t temp = reuse(var);
			
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

		void ByteCodeGenerator::write_variable(Tree::Variable *var, var_t value)
		{
			if(var->type == Tree::Variable::Heap)
			{
				var_t heap;

				if(var->owner != scope)
				{
					heap = create_var();

					gen<LookupOp>(heap, block->heap_array_var, scope->referenced_scopes.index_of(var->owner));
				}
				else
					heap = block->heap_var;

				gen<SetHeapVarOp>(heap, var->loc, value);
			}
			else
			{
				gen<MoveOp>(ref(var), value);
			}
		}
		
		void ByteCodeGenerator::convert_assignment(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::AssignmentNode *)basic_node;
			
			switch(node->left->type())
			{
				case Tree::SimpleNode::Variable:
				{
					auto variable = (Tree::VariableNode *)node->left;

					if(variable->var->type == Tree::Variable::Heap)
					{
						var_t temp = reuse(var);

						to_bytecode(node->right, temp);
						
						write_variable(variable->var, temp);

						if(is_var(var))
							gen<MoveOp>(var, temp);
					}
					else
					{
						to_bytecode(node->right, ref(variable->var));
					
						if(is_var(var))
							gen<MoveOp>(var, ref(variable->var));
					}
					
					return;
				}
				
				case Tree::SimpleNode::IVar:
				{
					auto variable = (Tree::IVarNode *)node->left;
					
					var_t temp = reuse(var);
					
					to_bytecode(node->right, temp);
						
					gen<SetIVarOp>(self_var(), variable->name, temp);

					if(is_var(var))
						gen<MoveOp>(var, temp);
					
					return;
				}
				
				case Tree::SimpleNode::Constant:
				{
					auto variable = (Tree::ConstantNode *)node->left;
					
					var_t value = reuse(var);
					var_t obj = create_var();
					
					if(variable->obj)
						to_bytecode(variable->obj, obj);
					else
						gen<LoadOp>(obj, Object::class_ref);
					
					to_bytecode(node->right, value);
					
					gen<SetConstOp>(obj, variable->name, value);

					if(is_var(var))
						gen<MoveOp>(var, value);
					
					return;	
				}	
				
				default:
					mirb_debug_abort("Unknown left hand expression");
			}
		}
		
		void ByteCodeGenerator::convert_self(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
				gen<MoveOp>(var, self_var());
		}
		
		void ByteCodeGenerator::convert_nil(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
				gen<LoadOp>(var, value_nil);
		}
		
		void ByteCodeGenerator::convert_true(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
				gen<LoadOp>(var, value_true);
		}
		
		void ByteCodeGenerator::convert_false(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
				gen<LoadOp>(var, value_false);
		}
		
		void ByteCodeGenerator::convert_array(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ArrayNode *)basic_node;
			
			if(!is_var(var))
			{
				for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
					to_bytecode(*i, 0);
				
				return;
			}
			
			size_t param = 0;

			VariableGroup group(this, node->entries.size);
			
			for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
			{
				to_bytecode(*i, group[param++]);
			}
			
			gen<ArrayOp>(var, group.size, group.use());	
		}
		
		void ByteCodeGenerator::convert_call(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::CallNode *)basic_node;
			
			var_t obj = reuse(var);
			
			to_bytecode(node->object, obj);
			
			size_t argc;
			var_t argv;

			var_t closure = call_args(node->arguments, node->block ? node->block->scope : 0, argc, argv);
			
			gen<CallOp>(var, obj, node->method, closure, node->block ? node->block->scope->final : 0, argc, argv);
		}
		
		void ByteCodeGenerator::convert_super(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::SuperNode *)basic_node;
			
			if(node->pass_args)
			{
				var_t closure = node->block ? block_arg(scope) : ref(scope->owner->block_parameter);
				
				// push arguments

				VariableGroup group(this, scope->owner->parameters.size);
				
				size_t param = 0;
				
				for(auto i = scope->owner->parameters.begin(); i != scope->owner->parameters.end(); ++i)
					gen<MoveOp>(group[param++], ref(*i));
				
				gen<SuperOp>(var, self_var(), ref(scope->module_var), ref(scope->name_var), closure, node->block ? node->block->scope->final : 0, group.size, group.use());
			}
			else
			{
				size_t argc;
				var_t argv;

				var_t closure = call_args(node->arguments, scope, argc, argv);
				
				gen<SuperOp>(var, self_var(), ref(scope->module_var), ref(scope->name_var), closure, node->block ? node->block->scope->final : 0, argc, argv);
			}
		}
		
		void ByteCodeGenerator::convert_if(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::IfNode *)basic_node;
			
			var_t temp = reuse(var);

			BasicBlock *label_else = create_block();
			BasicBlock *label_end = create_block();
			BasicBlock *body = create_block();
			
			to_bytecode(node->left, temp);
			
			if(node->inverted)
				gen_if(label_else, temp);
			else
				gen_unless(label_else, temp);
			
			split(body);
			
			if(is_var(var))
			{
				var_t result_left = create_var();
				var_t result_right = create_var();

				to_bytecode(node->middle, result_left);
				
				gen<MoveOp>(var, result_left);
				gen_branch(label_end);

				gen(label_else);

				to_bytecode(node->right, result_right);
				
				gen<MoveOp>(var, result_right);
				split(label_end);
			}
			else
			{
				to_bytecode(node->middle, 0);
				
				gen_branch(label_end);

				gen(label_else);

				to_bytecode(node->right, 0);

				split(label_end);
			}
		}
		
		void ByteCodeGenerator::convert_group(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::GroupNode *)basic_node;
			
			if(node->statements.empty())
			{
				if (is_var(var))
					gen<LoadOp>(var, value_nil);
				
				return;
			}
			
			for(auto i = node->statements.begin(); i != node->statements.end(); ++i)
			{
				to_bytecode(*i, i().entry.next ? 0 : var);
			}
		}
		
		void ByteCodeGenerator::convert_return(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ReturnNode *)basic_node;
			
			var_t temp = reuse(var);
			
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
		
		void ByteCodeGenerator::convert_break(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BreakNode *)basic_node;
			
			var_t temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<UnwindBreakOp>(temp, scope->parent->final, scope->break_id);
		}
		
		void ByteCodeGenerator::convert_next(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::NextNode *)basic_node;
			
			var_t temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<MoveOp>(block->return_var, temp);
			branch(block->epilog);
		}
		
		void ByteCodeGenerator::convert_redo(Tree::Node *basic_node, var_t var)
		{
			branch(body);
		}
		
		void ByteCodeGenerator::convert_class(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ClassNode *)basic_node;
			
			var_t super;
			
			if(node->super)
			{
				super = reuse(var);
				
				to_bytecode(node->super, super);
			}
			else
				super = 0;
			
			gen<ClassOp>(var, self_var(), node->name, super, compile(node->scope));
		}
		
		void ByteCodeGenerator::convert_module(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ModuleNode *)basic_node;
			
			gen<ModuleOp>(var, self_var(), node->name, compile(node->scope));
		}
		
		void ByteCodeGenerator::convert_method(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::MethodNode *)basic_node;
			
			gen<MethodOp>(self_var(), node->name, defer(node->scope));
			
			if(is_var(var))
				gen<LoadOp>(var, value_nil);
		}
		
		void ByteCodeGenerator::convert_handler(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::HandlerNode *)basic_node;
			
			scope->require_exceptions = true; // duh
			
			/*
			 * Allocate and setup the new exception block
			 */
			ExceptionBlock *exception_block = new ExceptionBlock;
			size_t index = block->final->exception_blocks.size();
			size_t old_index = block->current_exception_block_id;
			
			exception_block->parent_index = old_index;
			exception_block->parent = block->current_exception_block;
			
			/*
			 * Check for ensure node
			 */
			if(node->ensure_group)
				exception_block->ensure_label.block = create_block();
			else
				exception_block->ensure_label.block = 0;
			
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
			to_bytecode(node->code, var);

			if(!node->rescues.empty())
			{
				/*
				 * Skip the rescue block
				 */
				BasicBlock *ok_label = create_block();
				gen_branch(ok_label);
				
				/*
				 * Output rescue nodes. TODO: Check for duplicate nodes
				 */
				for(auto i = node->rescues.begin(); i != node->rescues.end(); ++i)
				{
					RuntimeExceptionHandler *handler = new RuntimeExceptionHandler;
					handler->type = RuntimeException;

					BasicBlock *handler_body = gen(create_block());

					handler->rescue_label.block = handler_body;

					exception_block->handlers.push(handler);
					
					gen<FlushOp>();
					
					to_bytecode(i().group, var);
					
					gen_branch(ok_label);
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
				split(exception_block->ensure_label.block);
				
				gen<FlushOp>();
					
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

		Block *ByteCodeGenerator::create()
		{
			scope = 0;

			block = new (memory_pool) Block(memory_pool);
			block->final = Collector::allocate<Mirb::Block>();

			return block;
		}
		
		Block *ByteCodeGenerator::to_bytecode(Tree::Scope *scope)
		{
			this->scope = scope;

			block = new (memory_pool) Block(memory_pool, scope);

			if(scope->final)
				block->final = scope->final;
			else
			{
				block->final = Collector::allocate<Mirb::Block>();
				scope->final = block->final;
			}
			
			scope->block = block;
			
			block->return_var = create_var();

			if(scope->referenced_scopes.size() > 0)
				block->heap_array_var = create_var();
			
			for(auto i = scope->zsupers.begin(); i != scope->zsupers.end(); ++i)
			{
				scope->require_args(*i);
			}

			body = create_block();
			block->epilog = create_block();
			
			if(scope->break_targets)
			{
				scope->require_exceptions = true;
				block->final->break_targets = new void *[scope->break_targets];
			}
			else
				block->final->break_targets = 0;
			
			BasicBlock *prolog = create_block();
			
			gen(prolog);
			
			split(body);

			if(scope->heap_vars)
				block->heap_var = create_var();

			to_bytecode(scope->group, block->return_var);
			
			split(block->epilog);

			gen<ReturnOp>(block->return_var);

			basic = prolog;
			
			var_t argv = scope->parameters.empty() ? 0 : create_var();

			gen<PrologueOp>(block->heap_array_var, block->heap_var, ref(block->scope->block_parameter), ref(block->scope->name_var), ref(block->scope->module_var), block->self_var, no_var, argv);

			if(argv)
			{
				size_t index = 0;

				for(auto i = scope->parameters.begin(); i != scope->parameters.end(); ++i, ++index)
				{
					if(i().type == Tree::Variable::Heap)
					{
						var_t value = create_var();

						gen<LookupOp>(value, argv, index);

						write_variable(*i, value);
					}
					else
						gen<LookupOp>(ref(*i), argv, index);
				}
			}
			
			block->epilog->next_block = 0;

			return block;
		}
		
		Mirb::Block *ByteCodeGenerator::compile(Tree::Scope *scope)
		{
			return Compiler::compile(scope, memory_pool);
		}

		Mirb::Block *ByteCodeGenerator::defer(Tree::Scope *scope)
		{
			return Compiler::defer(scope, memory_pool);
		}

		bool ByteCodeGenerator::has_ensure_block(Block *block)
		{
			ExceptionBlock *exception_block = block->current_exception_block;
			
			while(exception_block)
			{
				if(exception_block->ensure_label.block != 0)
					return true;

				exception_block = exception_block->parent;
			}
			
			return false;
		}
		
		BasicBlock *ByteCodeGenerator::create_block()
		{
			return new (memory_pool) BasicBlock(memory_pool, *block);
		}
		
		var_t ByteCodeGenerator::self_var()
		{
			if(!is_var(block->self_var))
				block->self_var = create_var();
			
			return block->self_var;
		}
		
		var_t ByteCodeGenerator::create_var()
		{
			return block->var_count++;
		}

		var_t ByteCodeGenerator::block_arg(Tree::Scope *scope)
		{
			var_t var = 0;
			
			if(scope)
			{
				var = create_var();
				auto *block_attach = defer(scope);
				
				VariableGroup group(this, scope->referenced_scopes.size());

				for(size_t i = 0; i < scope->referenced_scopes.size(); ++i)
				{
					if(scope->referenced_scopes[i] == this->scope)
						gen<MoveOp>(group[i], block->heap_var);
					else
						gen<LookupOp>(group[i], block->heap_array_var, scope->referenced_scopes.index_of(scope->referenced_scopes[i]));
				}

				gen<ClosureOp>(var, self_var(), ref(this->scope->name_var), ref(this->scope->module_var), block_attach, group.size, group.use());
			}
			
			return var;
		}
		
		var_t ByteCodeGenerator::call_args(Tree::CountedNodeList &arguments, Tree::Scope *scope, size_t &argc, var_t &argv)
		{
			if(!arguments.empty())
			{
				VariableGroup group(this, arguments.size);

				size_t param = 0;
				
				for(auto i = arguments.begin(); i != arguments.end(); ++i)
					to_bytecode(*i, group[param++]);

				argc = group.size;
				argv = group.use();
			}
			else
			{
				argc = 0;
				argv = 0;
			}
			
			return block_arg(scope);
		}
	};
};
