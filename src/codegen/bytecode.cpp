#include "bytecode.hpp"
#include "../tree/nodes.hpp"
#include "../tree/tree.hpp"
#include "../compiler.hpp"
#include "../block.hpp"
#include "../classes/fixnum.hpp"
#include "../classes/array.hpp"

#ifdef MIRB_DEBUG_COMPILER
	#include "printer.hpp"
#endif

namespace Mirb
{
	namespace CodeGen
	{
		VariableGroup::VariableGroup(ByteCodeGenerator *bcg, size_t size) : bcg(bcg), size(size)
		{
			if(size)
			{
				var = bcg->var_count;
				bcg->var_count += size;
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
			&ByteCodeGenerator::convert_data,
			&ByteCodeGenerator::convert_heredoc,
			&ByteCodeGenerator::convert_interpolate,
			&ByteCodeGenerator::convert_integer,
			&ByteCodeGenerator::convert_float,
			&ByteCodeGenerator::convert_variable,
			&ByteCodeGenerator::convert_cvar,
			&ByteCodeGenerator::convert_ivar,
			&ByteCodeGenerator::convert_global,
			&ByteCodeGenerator::convert_constant,
			&ByteCodeGenerator::convert_unary_op,
			&ByteCodeGenerator::convert_boolean_not,
			&ByteCodeGenerator::convert_binary_op,
			&ByteCodeGenerator::convert_assignment,
			&ByteCodeGenerator::convert_self,
			&ByteCodeGenerator::convert_nil,
			&ByteCodeGenerator::convert_true,
			&ByteCodeGenerator::convert_false,
			&ByteCodeGenerator::convert_symbol,
			&ByteCodeGenerator::convert_range,
			&ByteCodeGenerator::convert_array,
			&ByteCodeGenerator::convert_hash,
			0, // Block
			0, // Invoke
			&ByteCodeGenerator::convert_call,
			&ByteCodeGenerator::convert_super,
			&ByteCodeGenerator::convert_if,
			&ByteCodeGenerator::convert_case,
			&ByteCodeGenerator::convert_loop,
			&ByteCodeGenerator::convert_group,
			0, // Void
			&ByteCodeGenerator::convert_return,
			&ByteCodeGenerator::convert_next,
			&ByteCodeGenerator::convert_break,
			&ByteCodeGenerator::convert_redo,
			&ByteCodeGenerator::convert_class,
			&ByteCodeGenerator::convert_module,
			&ByteCodeGenerator::convert_method,
			&ByteCodeGenerator::convert_alias,
			0, // Rescue
			&ByteCodeGenerator::convert_handler,
			0, // Splat
			&ByteCodeGenerator::convert_multiple_expressions
		};
		
		void ByteCodeGenerator::convert_data(Tree::Node *basic_node, var_t var)
		{
			if(!is_var(var))
				return;

			auto node = (Tree::DataNode *)basic_node;

			switch(node->result_type)
			{
				case Value::Symbol:
					gen<LoadSymbolOp>(var, symbol_pool.get(node->data.data, node->data.length));
					break;
					
				case Value::String:
					gen_string(var, node->data);
					break;

				case Value::Regexp:
					gen_regexp(var, node->data);
					location(&node->range);
					break;

				case Value::Array:
				{
					var_t array = reuse(var);
					var_t element = create_var();

					gen<ArrayOp>(array, 0, 0);

					Array::parse(node->data.data, node->data.length, [&](const std::string &str){
						InterpolateData::Entry data;
						data.data = (const char_t *)str.data();
						data.length = str.length();
						gen_string(element, data);
						gen<PushOp>(array, element);
					});

					break;
				}

				default:
					mirb_debug_abort("Unknown data type");
			}
		}
		
		void ByteCodeGenerator::convert_heredoc(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::HeredocNode *)basic_node;

			to_bytecode(node->data, var);
		}
		
		void ByteCodeGenerator::convert_interpolate(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::InterpolateNode *)basic_node;
			
			size_t param_count = 0;

			for(auto i = node->pairs.begin(); i != node->pairs.end(); ++i)
			{
				if(i().string.length)
					param_count++;
				
				param_count++;
			}
			
			if(node->data.length)
				param_count++;
			
			VariableGroup group(this, param_count);

			size_t param = 0;
			
			for(auto i = node->pairs.begin(); i != node->pairs.end(); ++i)
			{
				if(i().string.length)
					gen_string(group[param++], i().string);
				
				to_bytecode(i().group, group[param++]);
			}
			
			if(node->data.length)
				gen_string(group[param++], node->data);
			
			gen<InterpolateOp>(var, group.size, group.use(), node->result_type);
		}
		
		void ByteCodeGenerator::convert_integer(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
			{
				auto node = (Tree::IntegerNode *)basic_node;
				
				gen<LoadFixnumOp>(var, Fixnum::from_int(node->value));
			}
		}
		
		void ByteCodeGenerator::convert_float(Tree::Node *basic_node, var_t var)
		{
			if(is_var(var))
			{
				auto node = (Tree::FloatNode *)basic_node;
				
				gen<LoadFloatOp>(var, node->value);
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

					gen<LookupOp>(heap, scope->referenced_scopes.index_of(var->owner));
				}
				else
					heap = heap_var;

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
		
		void ByteCodeGenerator::convert_cvar(Tree::Node *basic_node, var_t var)
		{
			if(!is_var(var))
				return;
			
			auto node = (Tree::CVarNode *)basic_node;
			
			gen<GetIVarOp>(var, node->name); // TODO: Fix cvars
		}
		
		void ByteCodeGenerator::convert_ivar(Tree::Node *basic_node, var_t var)
		{
			if(!is_var(var))
				return;
			
			auto node = (Tree::IVarNode *)basic_node;
			
			gen<GetIVarOp>(var, node->name);
		}
		
		void ByteCodeGenerator::convert_global(Tree::Node *basic_node, var_t var)
		{
			if(!is_var(var))
				return;
			
			auto node = (Tree::GlobalNode *)basic_node;
			
			gen<GetGlobalOp>(var, node->name);
		}
		
		void ByteCodeGenerator::convert_constant(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ConstantNode *)basic_node;
			
			if(node->obj)
			{
				to_bytecode(node->obj, var);
				gen<GetScopedConstOp>(var, var, node->name);
			}
			else if(node->top_scope)
			{
				gen<LoadObjectOp>(var);
				gen<GetScopedConstOp>(var, var, node->name);
			}
			else
			{
				gen<GetConstOp>(var, node->name);
			}

			location(node->range);
		}
		
		void ByteCodeGenerator::convert_unary_op(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::UnaryOpNode *)basic_node;
			
			var_t temp = reuse(var);
			
			to_bytecode(node->value, temp);
			
			gen<CallOp>(var, temp, Symbol::from_string(Lexeme::names[node->op].c_str()), no_var, (size_t)0, no_var);
			location(node->range);
		}
		
		void ByteCodeGenerator::convert_boolean_not(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BooleanNotNode *)basic_node;
			
			if(is_var(var))
			{
				var_t temp = reuse(var);
				
				to_bytecode(node->value, temp);
				
				Label *label_true = create_label();
				Label *label_end = create_label();
				Label *label_false = create_label();
				
				gen_if(label_true, temp);

				gen(label_false);
				gen<LoadTrueOp>(var);
				gen_branch(label_end);

				gen(label_true);
				gen<LoadFalseOp>(var);
				
				gen(label_end);
			}
			else
				to_bytecode(node->value, no_var);
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
			
			gen<CallOp>(var, left, Symbol::from_string(Lexeme::names[node->op].c_str()), no_var, group.size, group.use());
			location(&node->range);
		}
		
		void ByteCodeGenerator::convert_boolean_op(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BinaryOpNode *)basic_node;
			
			to_bytecode(node->left, var);
			
			Label *label_end = create_label();
			Label *body = create_label();
			
			if(node->op == Lexeme::KW_OR || node->op == Lexeme::LOGICAL_OR)
				gen_if(label_end, var);
			else
				gen_unless(label_end, var);

			gen(body);
			
			to_bytecode(node->right, var);
			
			gen(label_end);
		}
		
		void ByteCodeGenerator::convert_multiple_assignment(Tree::MultipleExpressionsNode *node, var_t rhs)
		{
			var_t temp = create_var();

			for(auto expr: node->expressions)
			{
				if(!expr->expression || expr->expression->type() == Tree::Node::Splat)
				{
					auto splat_node = static_cast<Tree::SplatNode *>(expr->expression);

					if(expr->expression && splat_node->expression)
						write_node(splat_node->expression, [&](var_t store) {
							gen<AssignArrayOp>(store, rhs, node->splat_index, node->expression_count);
						}, temp);
				}
				else
				{
					write_node(expr->expression, [&](var_t store) {
						gen<AssignOp>(store, rhs, expr->index, expr->size);
					}, temp);
				}
			}
		}
		
		void ByteCodeGenerator::convert_assignment(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::AssignmentNode *)basic_node;

			var_t result = write_node(node->left, [&](var_t store) {
				to_bytecode(node->right, store);
			}, var);

			if(var != no_var)
				gen<MoveOp>(var, result);
		}

		void ByteCodeGenerator::convert_self(Tree::Node *, var_t var)
		{
			if(is_var(var))
				gen<SelfOp>(var);
		}
		
		void ByteCodeGenerator::convert_nil(Tree::Node *, var_t var)
		{
			if(is_var(var))
				gen<LoadNilOp>(var);
		}
		
		void ByteCodeGenerator::convert_true(Tree::Node *, var_t var)
		{
			if(is_var(var))
				gen<LoadTrueOp>(var);
		}
		
		void ByteCodeGenerator::convert_false(Tree::Node *, var_t var)
		{
			if(is_var(var))
				gen<LoadFalseOp>(var);
		}
		
		void ByteCodeGenerator::convert_symbol(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::SymbolNode *)basic_node;

			if(is_var(var))
				gen<LoadSymbolOp>(var,  node->symbol);
		}
		
		void ByteCodeGenerator::convert_range(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::RangeNode *)basic_node;
			
			if(is_var(var))
			{
				var_t low = reuse(var);
				var_t high = create_var();
				
				to_bytecode(node->left, low);
				to_bytecode(node->right, high);

				gen<RangeOp>(var, low, high, node->exclusive);
			}
		}
		
		void ByteCodeGenerator::convert_array(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ArrayNode *)basic_node;

			if(node->variadic)
			{
				if(var == no_var)
				{
					for(auto expr: node->entries)
					{
						if(expr->type() == Tree::Node::Splat)
							to_bytecode(static_cast<Tree::SplatNode *>(expr)->expression, no_var);
						else
							to_bytecode(expr, no_var);
					}

					return;
				}

				gen<ArrayOp>(var, 0, 0);
			
				var_t temp = create_var();

				for(auto expr: node->entries)
				{
					if(expr->type() == Tree::Node::Splat)
					{
						to_bytecode(static_cast<Tree::SplatNode *>(expr)->expression, temp);
						gen<PushArrayOp>(var, temp);
					}
					else
					{
						to_bytecode(expr, temp);
						gen<PushOp>(var, temp);
					}
				}

				return;
			}

			
			if(!is_var(var))
			{
				for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
					to_bytecode(*i, no_var);
				
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
		
		void ByteCodeGenerator::convert_hash(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::HashNode *)basic_node;
			
			if(!is_var(var))
			{
				for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
					to_bytecode(*i, no_var);
				
				return;
			}
			
			size_t param = 0;

			VariableGroup group(this, node->entries.size);
			
			for(auto i = node->entries.begin(); i != node->entries.end(); ++i)
			{
				to_bytecode(*i, group[param++]);
			}
			
			gen<HashOp>(var, group.size, group.use());
			location(&node->range);
		}
		
		void ByteCodeGenerator::convert_call(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::CallNode *)basic_node;
			
			size_t argc;
			var_t argv;

			var_t closure = call_args(node, node->block ? node->block->scope : nullptr, argc, argv, var);
			
			var_t obj = reuse(var);
			
			to_bytecode(node->object, obj);
			
			if(node->variadic)
				gen<VariadicCallOp>(var, obj, node->method, closure, argv);
			else
				gen<CallOp>(var, obj, node->method, closure, argc, argv);

			location(node->range);
		}
		
		void ByteCodeGenerator::convert_super(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::SuperNode *)basic_node;
			
			if(node->pass_args)
			{
				var_t closure = node->block ? block_arg(node->block->scope, var) : ref(scope->owner->block_parameter);
				
				// push arguments

				VariableGroup group(this, scope->owner->parameters.size());
				
				size_t param = 0;
				
				for(auto i = scope->owner->parameters.begin(); i != scope->owner->parameters.end(); ++i)
					gen<MoveOp>(group[param++], ref(*i)); // TODO: Fix references to heap variables and check how array and block parameters should be handled
				
				gen<SuperOp>(var, closure, group.size, group.use());
				location(node->range);
			}
			else
			{
				size_t argc;
				var_t argv;

				var_t closure = call_args(node, scope, argc, argv, var);
				
				if(node->variadic)
					gen<VariadicSuperOp>(var, closure, argv);
				else
					gen<SuperOp>(var, closure, argc, argv);

				location(node->range);
			}
		}
		
		void ByteCodeGenerator::convert_if(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::IfNode *)basic_node;
			
			var_t temp = reuse(var);

			Label *label_else = create_label();
			Label *label_end = create_label();
			Label *body = create_label();
			
			to_bytecode(node->left, temp);
			
			if(node->inverted)
				gen_if(label_else, temp);
			else
				gen_unless(label_else, temp);
			
			gen(body);
			
			to_bytecode(node->middle, var);
				
			gen_branch(label_end);

			gen(label_else);

			to_bytecode(node->right, var);

			gen(label_end);
		}
		
		void ByteCodeGenerator::convert_case(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::CaseNode *)basic_node;
			
			var_t temp = reuse(var);
			var_t test = create_var();

			Label *label_end = create_label();
			
			if(node->value)
			{
				to_bytecode(node->value, temp);

				for(auto clause: node->clauses)
				{
					Label *skip = create_label();

					if(clause->pattern->type() == Tree::Node::MultipleExpressions)
					{
						Label *run = create_label();

						auto multi = static_cast<Tree::MultipleExpressionsNode *>(clause->pattern);
						
						for(auto expr: multi->expressions) // TODO: Handle splat expressions
						{
							to_bytecode(expr->expression, test);
			
							gen<CallOp>(test, temp, Symbol::get("==="), no_var, 1, test);
							location(&clause->range);
							gen_if(run, test);
						}
						
						gen_branch(skip);
						gen(run);
					}
					else
					{
						to_bytecode(clause->pattern, test);
			
						gen<CallOp>(test, temp, Symbol::get("==="), no_var, 1, test);
						location(&clause->range);
						gen_unless(skip, test);
					}

					to_bytecode(clause->group, temp);
					gen_branch(label_end);

					gen(skip);
				}
			}
			else
			{
				for(auto clause: node->clauses)
				{
					Label *skip = create_label();

					to_bytecode(clause->pattern, test);
					gen_unless(skip, test);

					to_bytecode(clause->group, temp);
					gen_branch(label_end);

					gen(skip);
				}
			}

			if(node->else_clause)
				to_bytecode(node->else_clause, temp);
			else if(var != no_var)
				gen<LoadNilOp>(var);

			gen(label_end);
		}
		
		void ByteCodeGenerator::convert_loop(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::LoopNode *)basic_node;

			if(node->trap_exceptions)
			{
				auto handler = new (memory_pool) Tree::HandlerNode;
		
				handler->code = node;
				handler->loop = node;
		
				return to_bytecode(handler, var);
			}
			
			node->label_start = create_label();
			node->label_body = create_label();
			node->label_end = create_label();
			
			gen(node->label_start);
			
			var_t temp = reuse(var);

			to_bytecode(node->condition, temp);
			
			gen(node->label_body);

			if(node->inverted)
				gen_if(node->label_end, temp);
			else
				gen_unless(node->label_end, temp);
			
			to_bytecode(node->body, temp);

			gen(body);
			gen_branch(node->label_start);
			gen(node->label_end);
			
			if(is_var(var))
				gen<LoadNilOp>(var);
		}
		
		void ByteCodeGenerator::convert_group(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::GroupNode *)basic_node;
			
			if(node->statements.empty())
			{
				if (is_var(var))
					gen<LoadNilOp>(var);
				
				return;
			}
			
			for(auto i = node->statements.begin(); i != node->statements.end(); ++i)
			{
				to_bytecode(*i, i().entry.next ? no_var : var);
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
				gen<UnwindReturnOp>(temp, scope->owner->final);
				location(node->range);

				//block_emmit_label(block, label);
			}
			else if(node->in_ensure)
			{
				gen<UnwindReturnOp>(temp, final);
				location(node->range);
			}
			else
			{
				gen<MoveOp>(return_var, temp);
				branch(epilog);
			}
		}
		
		void ByteCodeGenerator::convert_break(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::BreakNode *)basic_node;
			
			var_t temp = reuse(var);
			
			to_bytecode(node->value, temp);

			if(node->target)
			{
				if(node->in_ensure)
					gen<UnwindBreakOp>(temp, final, 0);
				else
					gen_branch(node->target->label_end);
			}
			else
			{
				gen<UnwindBreakOp>(temp, scope->parent->final, scope->break_dst);
				location(node->range);
			}
		}
		
		void ByteCodeGenerator::convert_next(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::NextNode *)basic_node;
			
			var_t temp = reuse(var);

			if(node->in_ensure)
				gen<UnwindNextOp>(return_var);
			else if(node->target)
				gen_branch(node->target->label_start);
			else
			{
				to_bytecode(node->value, temp);
			
				gen<MoveOp>(return_var, temp);
				branch(epilog);
			}
		}
		
		void ByteCodeGenerator::convert_redo(Tree::Node *basic_node, var_t)
		{
			auto node = (Tree::RedoNode *)basic_node;

			if(node->in_ensure)
				gen_unwind_redo(node->target ? node->target->label_body : body);
			else if(node->target)
				gen_branch(node->target->label_body);
			else
				branch(body);
		}
		
		void ByteCodeGenerator::convert_class(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ClassNode *)basic_node;

			if(node->singleton)
			{
				var_t singleton = reuse(var);
			
				to_bytecode(node->singleton, singleton);

				gen<SingletonClassOp>(var, singleton, compile(node->scope));
				location(node->range);
			}
			else
			{
				var_t super;
			
				if(node->super)
				{
					super = reuse(var);
				
					to_bytecode(node->super, super);
				}
				else
					super = no_var;
			
				var_t temp = no_var;

				if(node->scoped)
				{
					temp = create_var();
					to_bytecode(node->scoped, temp);
				}
				else if(node->top_scope)
				{
					temp = create_var();
					gen<LoadObjectOp>(temp);
				}

				gen<ClassOp>(var, temp, node->name, super, compile(node->scope));
				location(node->range);
			}
		}
		
		void ByteCodeGenerator::convert_module(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::ModuleNode *)basic_node;

			var_t temp = no_var;
			
			if(node->scoped)
			{
				temp = reuse(var);
				to_bytecode(node->scoped, temp);
			}
			else if(node->top_scope)
			{
				temp = reuse(var);
				gen<LoadObjectOp>(temp);
			}

			gen<ModuleOp>(var, temp, node->name, compile(node->scope));
			location(node->range);
		}
		
		void ByteCodeGenerator::convert_method(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::MethodNode *)basic_node;

			if(node->singleton)
			{
				var_t temp = reuse(var);

				to_bytecode(node->singleton, temp);

				gen<SingletonMethodOp>(temp, node->name, defer(node->scope));
				location(node->range);
			}
			else
				gen<MethodOp>(node->name, defer(node->scope));
			
			if(is_var(var))
				gen<LoadNilOp>(var);
		}
		
		void ByteCodeGenerator::convert_alias(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::AliasNode *)basic_node;
			
			var_t new_name = reuse(var);
			var_t old_name = create_var();
			
			to_bytecode(node->new_name, new_name);
			to_bytecode(node->old_name, old_name);

			gen<AliasOp>(new_name, old_name);
			location(&node->range);

			if(is_var(var))
				gen<LoadNilOp>(var);
		}
		
		void ByteCodeGenerator::convert_handler(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::HandlerNode *)basic_node;
			
			/*
			 * Allocate and setup the new exception block
			 */
			ExceptionBlock *exception_block = new ExceptionBlock;
			exception_block->parent = current_exception_block;
			
			/*
			 * Check for ensure node
			 */
			if(node->ensure_group)
				exception_block->ensure_label.label = create_label();
			else
				exception_block->ensure_label.label = 0;
			
			exception_block->loop = 0;
			
			exception_blocks.push(exception_block);
			
			/*
			 * Use the new exception block
			 */
			current_exception_block = exception_block;
			gen<HandlerOp>(exception_block);
			
			/*
			 * Output the regular code
			 */
			to_bytecode(node->code, var);
			
			gen<HandlerOp>(exception_block->parent);

			if(!node->rescues.empty())
			{
				/*
				 * Skip the rescue block
				 */
				Label *ok_label = create_label();
				gen_branch(ok_label);
				
				/*
				 * Output rescue nodes. TODO: Check for duplicate nodes
				 */
				for(auto i = node->rescues.begin(); i != node->rescues.end(); ++i)
				{
					ExceptionHandler *handler;

					var_t temp = var;

					if(i().pattern)
					{
						temp = reuse(temp);

						auto rescue =  new FilterExceptionHandler;

						rescue->type = FilterException;

						Label *test_body = gen(create_label());

						rescue->test_label.label = test_body;
						rescue->result = temp;
						
						to_bytecode(i().pattern, temp);

						gen<UnwindFilterOp>();
					
						handler = rescue;
					}
					else
					{
						handler = new ExceptionHandler;
						handler->type = StandardException;
					}
					
					Label *handler_body = gen(create_label());

					handler->rescue_label.label = handler_body;

					if(i().var)
					{
						write_node(i().var, [&](var_t store) {
							handler->var = store;
						}, temp);
					}
					else
						handler->var = no_var;

					exception_block->handlers.push(handler);

					to_bytecode(i().group, var);
					
					gen_branch(ok_label);
				}
				
				gen(ok_label);
			}
			else if(node->loop)
			{
				exception_block->loop = new LoopHandler;
				exception_block->loop->next_label.label = node->loop->label_start;
				exception_block->loop->break_label.label = node->loop->label_end;
			}
			
			/*
			 * Restore the old exception frame
			 */
			current_exception_block = exception_block->parent;
			
			if(node->else_group)
				to_bytecode(node->else_group, no_var);

			/*
			 * Check for ensure node
			 */
			if(node->ensure_group)
			{
				gen(exception_block->ensure_label.label);
				
				//gen<FlushOp>(); Flush
					
				/*
				 * Output ensure node
				 */
				to_bytecode(node->ensure_group, no_var);
				
				gen<UnwindEnsureOp>();
			}
		}
		
		void ByteCodeGenerator::convert_multiple_expressions(Tree::Node *basic_node, var_t var)
		{
			auto node = (Tree::MultipleExpressionsNode *)basic_node;

			if(var == no_var)
			{
				for(auto expr: node->expressions)
				{
					if(expr->expression->type() == Tree::Node::Splat)
						to_bytecode(static_cast<Tree::SplatNode *>(expr->expression)->expression, no_var);
					else
						to_bytecode(expr->expression, no_var);
				}

				return;
			}

			gen<ArrayOp>(var, 0, 0);
			
			var_t temp = create_var();

			for(auto expr: node->expressions)
			{
				if(expr->expression->type() == Tree::Node::Splat)
				{
					to_bytecode(static_cast<Tree::SplatNode *>(expr->expression)->expression, temp);
					gen<PushArrayOp>(var, temp);
				}
				else
				{
					to_bytecode(expr->expression, temp);
					gen<PushOp>(var, temp);
				}
			}
		}
		
		void ByteCodeGenerator::early_finalize(Block *block, Tree::Scope *scope)
		{
			block->min_args = 0;
			block->max_args = 0;

			for(size_t i = 0; i < scope->parameters.size(); ++i)
			{
				block->max_args++;

				if(!scope->parameters[i]->default_value)
					block->min_args++;
			}
			
			if(scope->type == Tree::Scope::Closure || scope->array_parameter)
				block->max_args = (size_t)-1;

			if(scope->range)
				block->range = new SourceLoc(*scope->range);
			else
				block->range = nullptr;

			final->blocks.push(block);
		};

		void ByteCodeGenerator::finalize()
		{
			if(strings.size())
			{
				final->string_count = strings.size();

				final->strings = new const char_t *[final->string_count];

				for(size_t i = 0; i < final->string_count; ++i)
					final->strings[i] = strings[i];
			}
			
			size_t ranges = source_locs.size();

			if(ranges)
			{
				final->ranges = (SourceLoc *)std::malloc(ranges * sizeof(SourceLoc));

				ranges = 0;
				
				for(auto i = source_locs.begin(); i != source_locs.end(); ++i, ++ranges)
				{
					final->ranges[ranges] = *i().second;

					final->source_location.set(i().first, &final->ranges[ranges]);
				}
			}
			else
				final->ranges = nullptr;

			const char *opcodes = (const char *)opcode.compact<Prelude::Allocator::Standard>();

			for(auto i = branches.begin(); i != branches.end(); ++i)
			{
				BranchOp *op = (BranchOp *)&opcodes[(*i).first];

				op->pos = (*i).second->pos;
			}

			if(exception_blocks.size())
			{
				final->exception_blocks = new ExceptionBlock *[exception_blocks.size()];
				final->exception_block_count = exception_blocks.size();

				for(size_t i = 0; i < final->exception_block_count; ++i)
				{
					ExceptionBlock *block = exception_blocks[i];
					
					final->exception_blocks[i] = block;
					
					if(block->ensure_label.label)
						block->ensure_label.address = block->ensure_label.label->pos;
					else
						block->ensure_label.address = -1;

					if(block->loop)
					{
						block->loop->next_label.address = block->loop->next_label.label->pos;
						block->loop->break_label.address = block->loop->break_label.label->pos;
					}

					for(auto i: block->handlers)
					{
						switch(i->type)
						{
							case FilterException:
							{
								FilterExceptionHandler *handler = (FilterExceptionHandler *)i;

								handler->test_label.address = handler->test_label.label->pos;
							}

							// Fallthrough

							case StandardException:
							{
								ExceptionHandler *handler = (ExceptionHandler *)i;

								handler->rescue_label.address = handler->rescue_label.label->pos;

								break;
							}

							default:
								break;
						}
					}
				}
			}

			scope->children.clear();

			final->scope = nullptr;
			final->opcodes = opcodes;
			final->var_words = var_count;
			final->executor = &evaluate_block;
		}

		ByteCodeGenerator::ByteCodeGenerator(MemoryPool memory_pool, Tree::Scope *scope) :
			scope(scope),
			current_exception_block(0),
			exception_blocks(memory_pool),
			strings(memory_pool),
			opcode(memory_pool),
			branches(memory_pool),
			source_locs(memory_pool),
			heap_var(no_var),
			var_count(scope->variable_list.size()),
			memory_pool(memory_pool)
		{
			Value::assert_valid(scope);

			#ifdef MIRB_DEBUG_COMPILER
				label_count = 0;
			#endif
		}

		Block *ByteCodeGenerator::generate()
		{
			Value::assert_valid(scope);

			if(scope->final)
				final = scope->final;
			else
			{
				final = Collector::allocate_pinned<Mirb::Block>(scope->document);
				scope->final = final;
			}
			
			return_var = create_var();

			for(auto i = scope->zsupers.begin(); i != scope->zsupers.end(); ++i)
			{
				scope->require_args(*i);
			}

			body = create_label();
			epilog = create_label();

			if(scope->break_targets)
				final->break_targets = new var_t[scope->break_targets];
			else
				final->break_targets = 0;
			
			if(scope->heap_vars)
			{
				heap_var = create_var();
				
				gen<CreateHeapOp>(heap_var, scope->heap_vars);
			}

			if(scope->block_parameter)
			{
				write_variable(scope->block_parameter, [&](var_t store){
					gen<BlockOp>(store);
				}, return_var);
			}

			final->min_args = 0;
			final->max_args = 0;

			for(size_t i = scope->parameters.size(); i-- >0;)
			{
				auto parameter = scope->parameters[i];

				final->max_args++;

				if(parameter->default_value)
				{
					write_variable(parameter, [&](var_t store){
						Label *skip = create_label();
						branches.push(BranchInfo(gen<LoadArgBranchOp>(store, i), skip));
						to_bytecode(parameter->default_value, store);
						gen(skip);
					}, return_var);
				}
				else
				{
					final->min_args++;

					write_variable(parameter, [&](var_t store){
						gen<LoadArgOp>(store, i);
					}, return_var);
				}
			}
			
			if(scope->type == Tree::Scope::Closure)
				final->max_args = (size_t)-1;

			if(scope->array_parameter)
			{
				final->max_args = (size_t)-1;

				if(scope->array_parameter->name)
					write_variable(scope->array_parameter, [&](var_t store){
						gen<LoadArrayArgOp>(store, scope->parameters.size());
					}, return_var);
			}

			gen(body);

			to_bytecode(scope->group, return_var);

			gen(epilog);

			gen<ReturnOp>(return_var);

			#ifdef MIRB_DEBUG_COMPILER
				CodeGen::ByteCodePrinter printer(this);

				std::cout << printer.print() << std::endl;
			#endif
			
			finalize();

			return final;
		}
		
		Mirb::Block *ByteCodeGenerator::compile(Tree::Scope *scope)
		{
			Mirb::Block *result = Compiler::compile(scope, memory_pool);
			
			final->blocks.push(result);

			return result;
		}

		Mirb::Block *ByteCodeGenerator::defer(Tree::Scope *scope)
		{
			Mirb::Block *result = Compiler::defer(scope);

			early_finalize(result, scope);
			
			return result;
		}

		Label *ByteCodeGenerator::create_label()
		{
			#ifdef MIRB_DEBUG_COMPILER
				return new (memory_pool) Label(label_count++);
			#else
				return new (memory_pool) Label;
			#endif
		}
		
		var_t ByteCodeGenerator::create_var()
		{
			return var_count++;
		}

		var_t ByteCodeGenerator::block_arg(Tree::Scope *scope, var_t break_dst)
		{
			var_t var = no_var;
			
			if(scope)
			{
				var = create_var();

				scope->break_dst = break_dst;

				auto *block_attach = defer(scope);
				
				VariableGroup group(this, scope->referenced_scopes.size());

				for(size_t i = 0; i < scope->referenced_scopes.size(); ++i)
				{
					if(scope->referenced_scopes[i] == this->scope)
						gen<MoveOp>(group[i], heap_var);
					else
						gen<LookupOp>(group[i], scope->referenced_scopes.index_of(scope->referenced_scopes[i]));
				}

				if(scope->break_id != Tree::Scope::no_break_id)
					final->break_targets[scope->break_id] = break_dst;

				gen<ClosureOp>(var, block_attach, group.size, group.use());
			}
			
			return var;
		}
		
		var_t ByteCodeGenerator::call_args(Tree::InvokeNode *node, Tree::Scope *scope, size_t &argc, var_t &argv, var_t break_dst)
		{
			Tree::CountedNodeList &arguments = node->arguments;

			if(scope)
				Value::assert_valid(scope);

			if(node->variadic)
			{
				argv = create_var();
				var_t temp = create_var();

				gen<ArrayOp>(argv, 0, 0);

				for(auto arg: node->arguments)
				{
					if(arg->type() == Tree::Node::Splat)
					{
						to_bytecode(static_cast<Tree::SplatNode *>(arg)->expression, temp);
						gen<PushArrayOp>(argv, temp);
					}
					else
					{
						to_bytecode(arg, temp);
						gen<PushOp>(argv, temp);
					}
				}
			}
			else
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
					argv = no_var;
				}
			}

			if(node->block_arg)
			{
				var_t result = create_var();
				to_bytecode(node->block_arg->node, result);
				return result;
			}
			else
				return block_arg(scope, break_dst);
		}
	};
};
