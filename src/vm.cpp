#include "vm.hpp"
#include "runtime.hpp"
#include "support.hpp"
#include "document.hpp"
#include "number.hpp"
#include "collector.hpp"
#include "generic/source-loc.hpp"
#include "classes/exceptions.hpp"
#include "classes/string.hpp"
#include "classes/regexp.hpp"
#include "classes/module.hpp"
#include "classes/array.hpp"
#include "classes/float.hpp"
#include "classes/range.hpp"
#include "classes/bignum.hpp"

namespace Mirb
{
	Frame *current_frame = 0;

#ifdef MIRB_THREADED
	#define OpContinue goto *labels[(size_t)*ip]
	#define OpPrologue static void *labels[] = {MIRB_OPCODES}; OpContinue;
	#define OpEpilogue
	#define Op(name) Op##name: { auto &op prelude_unused = *(CodeGen::name##Op *)ip; ip += sizeof(CodeGen::name##Op);
	#define EndOp OpContinue; }
#else
	#define OpContinue goto execute_instruction
	#define OpPrologue while(true) { execute_instruction: switch(*ip) {
	#define OpEpilogue default: mirb_debug_abort("Unknown opcode"); } }
	#define Op(name) case CodeGen::Opcode::name: { auto &op prelude_unused = *(CodeGen::name##Op *)ip; ip += sizeof(CodeGen::name##Op);
	#define EndOp break; }
#endif

#define DeepOp(name) Op(name) frame.ip = ip;
	
	size_t Frame::var_count(Block *code)
	{
		return code->var_words;
	}

	CodeGen::UnwindEnsureOp dummy_ensure;

	value_t evaluate_block(Frame &frame)
	{
		const char *ip_start = frame.code->opcodes;
		const char *ip = ip_start;
		
		ExceptionBlock *current_exception_block = nullptr;
		ExceptionBlock *loop_exception_block = nullptr; // Does not need initialization
		size_t loop_handler = 0; // Does not need initialization

#ifdef __GNUC__
		value_t storage[frame.code->var_words];
		value_t *vars = storage;
#else
		value_t *vars = (value_t *)alloca(frame.code->var_words * sizeof(value_t));
#endif

		frame.vars = vars;

		for(size_t i = 0; i < frame.code->var_words; ++i)
			vars[i] = value_nil;

		start_loop:
		try {

		OpPrologue

		Op(LoadArg)
			if(frame.argc > op.arg)
				vars[op.var] = frame.argv[op.arg];
		EndOp
			
		Op(LoadArgFloat)
			size_t pos = op.prev_reg + std::min(frame.argc - frame.code->min_args, op.prev_def);
		
			if(frame.argc > pos)
				vars[op.var] = frame.argv[pos];
		EndOp
			
		Op(LoadArrayArg)
			if(frame.argc > op.from_arg)
			{
				auto array = Collector::allocate<Array>();
				array->vector.push_entries(frame.argv + op.from_arg, frame.argc - op.from_arg);
				vars[op.var] = array;
			}
			else
				vars[op.var] = Collector::allocate<Array>();
		EndOp
			
		Op(LoadArgBranch)
			mirb_debug_assert(op.req_args >= op.arg);

			if(frame.argc >= op.req_args)
			{
				vars[op.var] = frame.argv[op.arg];
				ip = ip_start + op.pos;
			}
		EndOp
			
		Op(Move)
			vars[op.dst] = vars[op.src];
		EndOp
			
		Op(LoadNil)
			vars[op.var] = value_nil;
		EndOp

		Op(LoadTrue)
			vars[op.var] = value_true;
		EndOp

		Op(LoadFalse)
			vars[op.var] = value_false;
		EndOp
			
		Op(LoadFixnum)
			vars[op.var] = op.num;
		EndOp
			
		Op(Return)
			return vars[op.var];
		EndOp

		DeepOp(Call)
			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			value_t obj = vars[op.obj];
			Symbol *name = op.method;
			
			Method *method = lookup(obj, name);

			value_t result = call_code(method->block, obj, name, method->scope, method->scopes, block, op.argc, &vars[op.argv]);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		Op(Branch)
			ip = ip_start + op.pos;
		EndOp

		Op(BranchUnless)
			if(!vars[op.var]->test())
				ip = ip_start + op.pos;
		EndOp

		Op(BranchIf)
			if(vars[op.var]->test())
				ip = ip_start + op.pos;
		EndOp

		Op(Closure)
			vars[op.var] = Support::create_closure(op.block, frame.obj, frame.name, frame.scope, op.argc, &vars[op.argv]);
		EndOp
			
		Op(LoadObject)
			vars[op.var] = context->object_class;
		EndOp

		DeepOp(Class)
			Class *super = op.super == no_var ? context->object_class : raise_cast<Class>(vars[op.super]);

			Module *self = define_class(op.scope == no_var ? frame.scope->first() : vars[op.scope], op.name, super);
			
			value_t result = call_code(op.block, self, op.name, frame.scope->copy_and_prepend(self), 0, value_nil, 0, nullptr);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SingletonClass)
			Class *self = singleton_class(vars[op.singleton]);
			
			value_t result = call_code(op.block, self, Symbol::get("singleton class"), frame.scope->copy_and_prepend(self), 0, value_nil, 0, nullptr);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Module)
			Module *self = define_module(op.scope == no_var ? frame.scope->first() : vars[op.scope], op.name);
		
			value_t result = call_code(op.block, self, op.name, frame.scope->copy_and_prepend(self), 0, value_nil, 0, nullptr);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Super)
			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			
			Method *method = lookup_super(frame.scope->first(), frame.name);

			value_t result = call_code(method->block, frame.obj, frame.name, method->scope, method->scopes, block, op.argc, &vars[op.argv]);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp
			
		DeepOp(VariadicCall)
			auto array = cast<Array>(vars[op.argv]);

			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			value_t obj = vars[op.obj];
			Symbol *name = op.method;
			
			Method *method = lookup(obj, name);

			value_t result = call_argv(method, obj, name, block, array->vector.size(), array->vector.raw());

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(VariadicSuper)
			auto array = cast<Array>(vars[op.argv]);

			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			
			Method *method = lookup_super(frame.scope->first(), frame.name);

			value_t result = call_argv(method, frame.obj, frame.name, block, array->vector.size(), array->vector.raw());

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp
			
		Op(Method)
			Support::define_method(frame.scope, op.name, op.block);
		EndOp

		DeepOp(SingletonMethod)
			Support::define_singleton_method(frame.scope, vars[op.singleton], op.name, op.block);
		EndOp
			
		Op(LoadFloat)
			vars[op.var] = Collector::allocate<Float>(op.value);
		EndOp
			
		Op(Lookup)
			vars[op.var] = (*frame.scopes)[op.index];
		EndOp

		Op(Self)
			vars[op.var] = frame.obj;
		EndOp
			
		Op(LoadSymbol)
			vars[op.var] = op.symbol;
		EndOp
			
		DeepOp(AssertBlock)
			value_t value = vars[op.var];

			if(value != value_nil)
				raise_cast<Proc>(value);
		EndOp
			
		Op(Block)
			vars[op.var] = frame.block;
		EndOp
			
		Op(Assign)
			auto array = cast<Array>(vars[op.array]);
		
			if(array->vector.size() > op.size)
				vars[op.var] = op.index < 0 ? array->vector[op.index + array->vector.size()] : array->vector[op.size];
			else
				vars[op.var] = value_nil;
		EndOp
			
		Op(AssignArray)
			auto array = cast<Array>(vars[op.array]);

			auto result = Collector::allocate<Array>();
		
			if(array->vector.size() > op.size)
				result->vector.push_entries(&array->vector[op.index], array->vector.size() - op.size);
			
			vars[op.var] = result;
		EndOp
			
		Op(Push)
			cast<Array>(vars[op.array])->vector.push(vars[op.value]);
		EndOp
			
		Op(PushArray)
			value_t value = vars[op.from];
			
			if(of_type<Array>(value))
				cast<Array>(vars[op.into])->vector.push(cast<Array>(value)->vector);
			else
				cast<Array>(vars[op.into])->vector.push(value);
		EndOp
			
		Op(CreateHeap)
			Tuple<> &heap = *Tuple<>::allocate(op.vars);

			for(size_t i = 0; i < op.vars; ++i)
				heap[i] = value_nil;

			vars[op.var] = &heap;
		EndOp

		Op(GetHeapVar)
			Tuple<> &heap = *static_cast<Tuple<> *>(vars[op.heap]);

			vars[op.var] = heap[op.index];
		EndOp

		Op(SetHeapVar)
			Tuple<> &heap = *static_cast<Tuple<> *>(vars[op.heap]);

			heap[op.index] = vars[op.var];
		EndOp

		Op(GetIVar)
			vars[op.var] = get_var(frame.obj, op.name);
		EndOp

		Op(SetIVar)
			set_var(frame.obj, op.name,  vars[op.var]);
		EndOp
			
		DeepOp(GetScopedConst)
			value_t result = get_scoped_const(vars[op.obj], op.name);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SetScopedConst)
			set_const(vars[op.obj], op.name,  vars[op.var]);
		EndOp
			
		DeepOp(GetConst)
			value_t result = get_const(frame.scope, op.name);

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SetConst)
			set_const(frame.scope->first(), op.name,  vars[op.var]);
		EndOp
			
		Op(Array)
			vars[op.var] = Support::create_array(op.argc, &vars[op.argv]);
		EndOp

		DeepOp(Hash)
			vars[op.var] = Support::create_hash(op.argc, &vars[op.argv]);
		EndOp

		Op(String)
			vars[op.var] = CharArray(op.str, op.size).to_string();
		EndOp

		DeepOp(Regexp)
			vars[op.var] = Regexp::allocate(CharArray(op.str, op.size));
		EndOp
			
		DeepOp(Range)
			vars[op.var] = Range::allocate(vars[op.low], vars[op.high], op.exclusive);
		EndOp

		DeepOp(Interpolate)
			vars[op.var] = Support::interpolate(op.argc, &vars[op.argv], op.result);
		EndOp
			
		DeepOp(Alias)
			Module::alias_method(frame.scope->first(), cast<Symbol>(vars[op.new_name]), cast<Symbol>(vars[op.old_name]));
		EndOp
			
		DeepOp(Undef)
			Module::undef_method(frame.scope->first(), cast<Symbol>(vars[op.name]));
		EndOp
			
		DeepOp(LoadBignum)
			vars[op.var] = new (collector) Bignum(Number(op.data, op.length));
		EndOp
			
		DeepOp(CaseBranch)
			if(Support::case_match(vars[op.value], vars[op.list]))
				ip = ip_start + op.pos;
		EndOp
		
		Op(Handler)
			current_exception_block = op.block;
		EndOp
			
		Op(UnwindEnsure)
			if(prelude_unlikely(frame.exception != 0))
				goto handle_exception;
		EndOp

		Op(UnwindFilter)
			goto exception_block_handler;
		EndOp

		DeepOp(UnwindReturn)
			throw InternalException(Collector::allocate<ReturnException>(Type::ReturnException, context->local_jump_error, String::get("Unhandled return from block"), backtrace(), op.code, vars[op.var]));
		EndOp

		DeepOp(UnwindBreak)
			throw InternalException(Collector::allocate<BreakException>(context->local_jump_error, String::get("Unhandled break from block"), backtrace(), op.code, vars[op.var], op.parent_dst));
		EndOp
			
		Op(UnwindNext)
			throw InternalException(Collector::allocate<NextException>(Type::NextException, context->local_jump_error, nullptr, nullptr, vars[op.var]));
		EndOp

		Op(UnwindRedo)
			throw InternalException(Collector::allocate<RedoException>(Type::RedoException, context->local_jump_error, nullptr, nullptr, op.pos));
		EndOp
			
		DeepOp(GetGlobal)
			vars[op.var] = get_global(op.name);
		EndOp

		DeepOp(SetGlobal)
			set_global(op.name, vars[op.var]);
		EndOp

		OpEpilogue

handle_exception:
		if(frame.exception->type() == Type::SystemStackError && stack_no_reserve(frame))
			throw InternalException(frame.exception);

		{
			if(!current_exception_block)
			{
				switch(frame.exception->type())
				{
					case Type::RedoException:
					{
						auto error = (RedoException *)frame.exception;
						
						frame.exception = 0;

						ip = ip_start + error->pos;
						OpContinue; // Restart block
					}
					break;

					case Type::NextException:
					{
						auto error = (NextException *)frame.exception;

						value_t result = error->value;
						frame.exception = 0;
						return result;
					}
					break;

					case Type::ReturnException:
					{
						auto error = (ReturnException *)frame.exception;

						if(error->target == frame.code)
						{
							value_t result = error->value;
							frame.exception = 0;
							return result;
						}
					}
					break;

					default:
						break;
				}

				throw InternalException(frame.exception);
			}
			
			loop_exception_block = current_exception_block;

			current_exception_block = current_exception_block->parent;

			for(loop_handler = 0; loop_handler < loop_exception_block->handlers.size(); ++loop_handler)
			{
				switch(loop_exception_block->handlers[loop_handler]->type)
				{
					case FilterException:
					{
						ip = ip_start + static_cast<FilterExceptionHandler *>(loop_exception_block->handlers[loop_handler])->test_label.address;
						OpContinue; // Execute test block

exception_block_handler: // The test block will jump back here
						auto list = try_cast<Array>(vars[static_cast<FilterExceptionHandler *>(loop_exception_block->handlers[loop_handler])->result]);

						if(!list)
							continue;

						for(size_t i = 0; i < list->size(); ++i)
						{
							auto klass = try_cast<Class>(list->get(i));

							if(klass && kind_of(klass, frame.exception))
								goto run_handler;
						}

						continue;
					}

					case StandardException:
					{
						if(!kind_of(context->standard_error, frame.exception))
							continue;
						else
							break;
					}
				}

				run_handler:
				
				auto handler = loop_exception_block->handlers[loop_handler];

				if(handler->var != no_var)
					vars[handler->var] = frame.exception;
					
				frame.exception = 0;

				ip = ip_start + handler->rescue_label.address;
				OpContinue; // Execute rescue block
			}

			if(loop_exception_block->loop)
			{
				switch(frame.exception->type())
				{
					case Type::BreakException:
					{
						frame.exception = 0;
						
						ip = ip_start + loop_exception_block->loop->break_label.address;
						OpContinue; // Exit loop
					}
					break;

					case Type::RedoException:
					{
						auto error = (RedoException *)frame.exception;

						frame.exception = 0;

						ip = ip_start + error->pos;
						OpContinue; // Restart loop
					}
					break;

					case Type::NextException:
					{
						frame.exception = 0;

						ip = ip_start + loop_exception_block->loop->next_label.address;
						OpContinue; // Try next iteration
					}
					break;

					default:
						break;
				}
			}

			current_exception_block = loop_exception_block->parent;

			if(loop_exception_block->ensure_label.address != (size_t)-1)
			{
				ip = ip_start + loop_exception_block->ensure_label.address;
				OpContinue; // Execute ensure block
			}
			else
				goto handle_exception; // Handle parent frame
		}

		} catch(InternalException e)
		{
			if(frame.exception) // We are rethrowing
				throw;

			auto break_exception = (BreakException *)e.value;

			if(prelude_unlikely(e.value->type() == Type::BreakException && break_exception->target == frame.code))
			{
				if(break_exception->dst != no_var)
					vars[break_exception->dst] = break_exception->value;

				goto start_loop;
			}
			
			frame.exception = e.value;

			ip = (const char *)&dummy_ensure;
			goto start_loop;
		}
	}
};
