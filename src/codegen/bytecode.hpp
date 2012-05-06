#pragma once
#include "../common.hpp"
#include "opcodes.hpp"
#include "../tree/nodes.hpp"
#include "../tree/tree.hpp"
#include <Prelude/JoiningBuffer.hpp>

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
		
		class Label
		{
			public:
				#ifdef MIRB_DEBUG_COMPILER
					size_t id;
					Label(size_t id) : id(id) {}
				#endif

				size_t pos;
		};

		class ByteCodeGenerator
		{
			private:
				void convert_string(Tree::Node *basic_node, var_t var);
				void convert_interpolated(Tree::Node *basic_node, var_t var);
				void convert_integer(Tree::Node *basic_node, var_t var);
				void convert_variable(Tree::Node *basic_node, var_t var);
				void convert_ivar(Tree::Node *basic_node, var_t var);
				void convert_global(Tree::Node *basic_node, var_t var);
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
				void convert_symbol(Tree::Node *basic_node, var_t var);
				void convert_range(Tree::Node *basic_node, var_t var);
				void convert_array(Tree::Node *basic_node, var_t var);
				void convert_hash(Tree::Node *basic_node, var_t var);
				void convert_call(Tree::Node *basic_node, var_t var);
				void convert_super(Tree::Node *basic_node, var_t var);
				void convert_if(Tree::Node *basic_node, var_t var);
				void convert_case(Tree::Node *basic_node, var_t var);
				void convert_loop(Tree::Node *basic_node, var_t var);
				void convert_group(Tree::Node *basic_node, var_t var);
				void convert_return(Tree::Node *basic_node, var_t var);
				void convert_break(Tree::Node *basic_node, var_t var);
				void convert_next(Tree::Node *basic_node, var_t var);
				void convert_redo(Tree::Node *basic_node, var_t var);
				void convert_class(Tree::Node *basic_node, var_t var);
				void convert_module(Tree::Node *basic_node, var_t var);
				void convert_method(Tree::Node *basic_node, var_t var);
				void convert_handler(Tree::Node *basic_node, var_t var);
				void convert_multiple_expressions(Tree::Node *basic_node, var_t var);

				static void (ByteCodeGenerator::*jump_table[Tree::SimpleNode::Types])(Tree::Node *basic_node, var_t var);
				
				void convert_multiple_assignment(Tree::MultipleExpressionsNode *node, var_t rhs);

				// Members

				Label *body; // The point after the prolog of the block.
				
				Tree::Scope *const scope;

				Label *epilog; // The end of the block
				var_t return_var;
				
				Mirb::Block *final;

				/*
				 * Exception related fields
				 */
				ExceptionBlock *current_exception_block;
				Vector<ExceptionBlock *, MemoryPool> exception_blocks;

				Vector<const char_t  *, MemoryPool> strings;
				
				JoiningBuffer<sizeof(MoveOp) * 32, MemoryPool> opcode;
				
				typedef std::pair<size_t, Label *> BranchInfo;
				typedef std::pair<size_t, Range *> SourceInfo;

				Vector<BranchInfo, MemoryPool> branches;
				Vector<SourceInfo, MemoryPool> source_locs;
				
				var_t self_var;
				var_t heap_var;

				void finalize();
				
				size_t var_count; // The total variable count.
				
				#ifdef MIRB_DEBUG_COMPILER
					size_t label_count; // Nicer label labeling...
				#endif
				
				size_t loc;
				
				Mirb::Block *compile(Tree::Scope *scope);
				Mirb::Block *defer(Tree::Scope *scope);
				
				void to_bytecode(Tree::Node *node, var_t var)
				{
					(this->*jump_table[node->type()])(node, var);
				}
				
				var_t reuse(var_t var)
				{
					// TODO: Make sure no code stores results in the returned variable
					return var == no_var ? create_var() : var;
				}
				
				var_t ref(Tree::Variable *var)
				{
					if(var)
						return var->loc;
					else
						return no_var;
				}
				
				void gen_string(var_t var, StringData::Entry data)
				{
					data = data.copy<Prelude::Allocator::Standard>();
					strings.push(data.data);
					gen<StringOp>(var, data);
				}
				
				void gen_if(Label *ltrue, var_t var)
				{
					branches.push(BranchInfo(gen<BranchIfOp>(var), ltrue));
				}
				
				void gen_unwind_redo(Label *body)
				{
					branches.push(BranchInfo(gen<UnwindRedoOp>(), body));
				}
				
				void gen_unless(Label *lfalse, var_t var)
				{
					branches.push(BranchInfo(gen<BranchUnlessOp>(var), lfalse));
				}
				
				void location(Range *range)
				{
					source_locs.push(SourceInfo(opcode.size(), range));
				}
				
				void gen_branch(Label *block)
				{
					branches.push(BranchInfo(gen<BranchOp>(), block));
				}
				
				void branch(Label *block)
				{
					gen_branch(block);
					
					//TODO: Eliminate code generation from this point. The block will never be reached
					gen(create_label());
				}
				
				template<typename F> var_t write_node(Tree::Node *lhs, F func, var_t temp)
				{
					switch(lhs->type())
					{
						case Tree::SimpleNode::MultipleExpressions:
						{
							var_t var = reuse(temp);
							var_t rhs = create_var();

							func(var);
							
							gen<ArrayOp>(rhs, 0, 0); // TODO: Add an opcode to convert into an array to avoid copying data
							gen<PushArrayOp>(rhs, var);
							
							convert_multiple_assignment((Tree::MultipleExpressionsNode *)lhs, rhs);

							return var;
						}
				
						case Tree::SimpleNode::Call:
						{
							auto node = (Tree::CallNode *)lhs;
							
							var_t var = reuse(temp);

							func(var);
							
							size_t argc = node->arguments.size + 1;

							VariableGroup group(this, argc);

							size_t param = 0;
							
							for(auto arg: node->arguments)
								to_bytecode(arg, group[param++]);

							gen<MoveOp>(group[param++], var);
							
							var_t obj = create_var();
			
							to_bytecode(node->object, obj);
			
							gen<CallOp>(var, obj, node->method, no_var, argc, group.use());
							location(node->range);

							return var;
						}
				
						case Tree::SimpleNode::Variable:
						{
							auto variable = (Tree::VariableNode *)lhs;

							return write_variable(variable->var, func, temp);
						}
				
						case Tree::SimpleNode::IVar:
						{
							auto variable = (Tree::IVarNode *)lhs;

							var_t var = reuse(temp);

							func(var);
					
							gen<SetIVarOp>(variable->name, var);

							return var;
						}
				
						case Tree::SimpleNode::Global:
						{
							auto variable = (Tree::GlobalNode *)lhs;
							
							var_t var = reuse(temp);

							func(var);
					
							gen<SetGlobalOp>(variable->name, var);
							
							return var;
						}
				
						case Tree::SimpleNode::Constant:
						{
							auto variable = (Tree::ConstantNode *)lhs;
					
							var_t var = reuse(temp);

							func(var);
					
							var_t obj = no_var;
					
							if(variable->obj)
							{
								obj = create_var();
								to_bytecode(variable->obj, obj);
							}
							else if(variable->top_scope)
							{
								obj = create_var();
								gen<LoadObjectOp>(obj);
							}
							
							if(obj == no_var)
								gen<SetConstOp>(variable->name, var);
							else
								gen<SetScopedConstOp>(obj, variable->name, var);

							location(variable->range);

							return var;	
						}	
				
						default:
							mirb_debug_abort("Unknown left hand expression");
					}
				}
				
				template<typename F> var_t write_variable(Tree::Variable *var, F func, var_t temp)
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

						var_t store = reuse(temp);

						func(store);

						gen<SetHeapVarOp>(heap, var->loc, store);

						return store;
					}
					else
					{
						func(ref(var));
						return ref(var);
					}
				}
		
				var_t block_arg(Tree::Scope *scope, var_t break_dst);
				var_t call_args(Tree::InvokeNode *node, Tree::Scope *scope, size_t &argc, var_t &argv, var_t break_dst);
				void early_finalize(Block *block, Tree::Scope *scope);

				friend class VariableGroup;
				friend class ByteCodePrinter;
			public:
				ByteCodeGenerator(MemoryPool memory_pool, Tree::Scope *scope);

				MemoryPool memory_pool;
				
				Label *gen(Label *block)
				{
					block->pos = opcode.size();

					return block;
				}
				
				bool is_var(var_t var)
				{
					return var != no_var;
				}
				
				template<class T, typename F> size_t alloc(F func)
				{
					size_t result = opcode.size();
					func(opcode.allocate(sizeof(T)));
					return result;
				}

				template<class T> size_t gen()
				{
					return alloc<T>([&](void *p) { new (p) T(); });
				}

				template<class T, typename Arg1> size_t gen(Arg1 arg1)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1)); });
				}

				template<class T, typename Arg1, typename Arg2> size_t gen(Arg1&& arg1, Arg2&& arg2)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2)); });
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3)); });
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4)); });
				}
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5)); });
				}

				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6)); });
				}
				
				template<class T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7> size_t gen(Arg1&& arg1, Arg2&& arg2, Arg3&& arg3, Arg4&& arg4, Arg5&& arg5, Arg6&& arg6, Arg7&& arg7)
				{
					return alloc<T>([&](void *p) { new (p) T(std::forward<Arg1>(arg1), std::forward<Arg2>(arg2), std::forward<Arg3>(arg3), std::forward<Arg4>(arg4), std::forward<Arg5>(arg5), std::forward<Arg6>(arg6), std::forward<Arg7>(arg7)); });
				}
				
				var_t create_var();
				Label *create_label();
				
				var_t read_variable(Tree::Variable *var);
				void write_variable(Tree::Variable *var, var_t value);

				Block *generate();
		};
	};
};
