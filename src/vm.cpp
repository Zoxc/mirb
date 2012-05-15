#include "vm.hpp"
#include "runtime.hpp"
#include "support.hpp"
#include "document.hpp"
#include "generic/source-loc.hpp"
#include "classes/exceptions.hpp"
#include "classes/string.hpp"
#include "classes/regexp.hpp"
#include "classes/module.hpp"
#include "classes/array.hpp"
#include "classes/float.hpp"
#include "classes/range.hpp"

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

	value_t evaluate_block(Frame &frame)
	{
		const char *ip_start = frame.code->opcodes;
		const char *ip = ip_start;

		ExceptionBlock *current_exception_block = nullptr;
		size_t current_handler;
		Exception *current_exception;

#ifdef __GNUC__
		value_t storage[frame.code->var_words];
		value_t *vars = storage;
#else
		value_t *vars = (value_t *)alloca(frame.code->var_words * sizeof(value_t));
#endif

		frame.vars = vars;

		for(size_t i = 0; i < frame.code->var_words; ++i)
			vars[i] = value_nil;

		OpPrologue

		Op(LoadArg)
			vars[op.var] = frame.argv[op.arg];
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
			if(frame.argc > op.arg)
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
			value_t result = vars[op.var];
			validate_return(result);
			return result;
		EndOp

		DeepOp(Call)
			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			value_t obj = vars[op.obj];
			Symbol *name = op.method;
			
			Method *method = lookup(obj, name);

			if(prelude_unlikely(!method))
				goto handle_call_exception;

			value_t result = call_code(method->block, obj, name, method->scope, block, op.argc, &vars[op.argv]);

			if(prelude_unlikely(result == value_raise))
				goto handle_call_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		Op(Branch)
			ip = ip_start + op.pos;
		EndOp

		Op(BranchUnless)
			if(!Value::test(vars[op.var]))
				ip = ip_start + op.pos;
		EndOp

		Op(BranchIf)
			if(Value::test(vars[op.var]))
				ip = ip_start + op.pos;
		EndOp

		Op(Closure)
			vars[op.var] = Support::create_closure(op.block, frame.obj, frame.name, frame.scope, op.argc, &vars[op.argv]);
		EndOp
			
		Op(LoadObject)
			vars[op.var] = auto_cast(context->object_class);
		EndOp

		DeepOp(Class)
			value_t super = op.super == no_var ? context->object_class : vars[op.super];

			Module *self = define_class(op.scope == no_var ? frame.scope->first() : vars[op.scope], op.name, auto_cast(super));
			
			if(prelude_unlikely(self == nullptr))
				goto handle_exception;

			value_t result = call_code(op.block, self, op.name, frame.scope->copy_and_prepend(self), value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SingletonClass)
			Class *self = singleton_class(vars[op.singleton]);
			
			if(prelude_unlikely(self == nullptr))
				goto handle_exception;

			value_t result = call_code(op.block, self, Symbol::get("singleton class"), frame.scope->copy_and_prepend(self), value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Module)
			Module *self = define_module(op.scope == no_var ? frame.scope->first() : vars[op.scope], op.name);
		
			if(prelude_unlikely(self == nullptr))
				goto handle_exception;

			value_t result = call_code(op.block, self, op.name, frame.scope->copy_and_prepend(self), value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Super)
			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			
			Method *method = lookup_super(frame.scope->first(), frame.name);

			if(prelude_unlikely(!method))
				goto handle_exception;

			value_t result = call_code(method->block, frame.obj, frame.name, method->scope, block, op.argc, &vars[op.argv]);

			if(prelude_unlikely(result == value_raise))
				goto handle_call_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp
			
		DeepOp(VariadicCall)
			auto array = cast<Array>(vars[op.argv]);

			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			value_t obj = vars[op.obj];
			Symbol *name = op.method;
			
			Method *method = lookup(obj, name);

			if(prelude_unlikely(!method))
				goto handle_call_exception;

			value_t result = call_argv(method->block, obj, name, method->scope, block, array->vector.size(), array->vector.raw());

			if(prelude_unlikely(result == value_raise))
				goto handle_call_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(VariadicSuper)
			auto array = cast<Array>(vars[op.argv]);

			value_t block = op.block_var != no_var ? vars[op.block_var] : value_nil;
			
			Method *method = lookup_super(frame.scope->first(), frame.name);

			if(prelude_unlikely(!method))
				goto handle_exception;

			value_t result = call_argv(method->block, frame.obj, frame.name, method->scope, block, array->vector.size(), array->vector.raw());

			if(prelude_unlikely(result == value_raise))
				goto handle_call_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp
			
		Op(Method)
			Support::define_method(frame.scope, op.name, op.block);
		EndOp

		DeepOp(SingletonMethod)
			if(prelude_unlikely(!Support::define_singleton_method(frame.scope, vars[op.singleton], op.name, op.block)))
				goto handle_exception;
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
			
			if(Value::of_type<Array>(value))
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
		
			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SetScopedConst)
			if((prelude_unlikely(set_const(vars[op.obj], op.name,  vars[op.var]) == value_raise)))
				goto handle_exception;
		EndOp
			
		DeepOp(GetConst)
			value_t result = get_const(frame.scope, op.name);
		
			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SetConst)
			if((prelude_unlikely(set_const(frame.scope->first(), op.name,  vars[op.var]) == value_raise)))
				goto handle_exception;
		EndOp
			
		Op(Array)
			vars[op.var] = Support::create_array(op.argc, &vars[op.argv]);
		EndOp

		DeepOp(Hash)
			value_t result = Support::create_hash(op.argc, &vars[op.argv]);
		
			if(prelude_unlikely(!result))
				goto handle_exception;

			vars[op.var] = result;
		EndOp

		Op(String)
			vars[op.var] = CharArray(op.str.data, op.str.length).to_string();
		EndOp

		Op(Regexp)
			value_t result = Regexp::allocate(CharArray(op.str.data, op.str.length));

			if(prelude_unlikely(!result))
				goto handle_exception;

			vars[op.var] = result;
		EndOp
			
		DeepOp(Range)
			value_t result = Range::allocate(vars[op.low], vars[op.high], op.exclusive);

			if(prelude_unlikely(!result))
				goto handle_exception;

			vars[op.var] = result;
		EndOp

		Op(Interpolate)
			vars[op.var] = Support::interpolate(op.argc, &vars[op.argv], op.result);
		EndOp
			
		DeepOp(Alias)
			value_t result = Module::alias_method(frame.scope->first(), auto_cast(vars[op.new_name]), auto_cast(vars[op.old_name]));

			if(prelude_unlikely(!result))
				goto handle_exception;
		EndOp
			
		Op(Handler)
			current_exception_block = op.block;
		EndOp
			
		Op(UnwindEnsure)
			if(prelude_unlikely(context->exception))
				goto handle_exception;
		EndOp

		Op(UnwindFilter)
			goto exception_block_handler;
		EndOp

		DeepOp(UnwindReturn)
			set_current_exception(Collector::allocate<ReturnException>(Value::ReturnException, context->local_jump_error, String::get("Unhandled return from block"), backtrace(), op.code, vars[op.var]));
			goto handle_exception;
		EndOp

		DeepOp(UnwindBreak)
			set_current_exception(Collector::allocate<BreakException>(context->local_jump_error, String::get("Unhandled break from block"), backtrace(), op.code, vars[op.var], op.parent_dst));
			goto handle_exception;
		EndOp
			
		Op(UnwindNext)
			set_current_exception(Collector::allocate<NextException>(Value::NextException, context->local_jump_error, value_nil, nullptr, vars[op.var]));
			goto handle_exception;
		EndOp

		Op(UnwindRedo)
			set_current_exception(Collector::allocate<RedoException>(Value::RedoException, context->local_jump_error, value_nil, nullptr, op.pos));
			goto handle_exception;
		EndOp
			
		Op(GetGlobal)
			vars[op.var] = get_global(op.name);
		EndOp

		Op(SetGlobal)
			set_global(op.name, vars[op.var]);
		EndOp

		OpEpilogue

handle_call_exception:
		{
			BreakException *exception = (BreakException *)context->exception;

			if(prelude_unlikely(exception->get_type() == Value::BreakException && exception->target == frame.code))
			{
				if(exception->dst != no_var)
					vars[exception->dst] = exception->value;

				set_current_exception(0);
				OpContinue;
			}
		}

		// Fallthrough

handle_exception:
		{
			current_exception = context->exception;

			if(!current_exception_block)
			{
				switch(current_exception->get_type())
				{
					case Value::RedoException:
					{
						auto error = (RedoException *)current_exception;

						set_current_exception(0);

						ip = ip_start + error->pos;
						OpContinue; // Restart block
					}
					break;

					case Value::NextException:
					{
						auto error = (NextException *)current_exception;

						value_t result = error->value;
						set_current_exception(0);
						return result;
					}
					break;

					case Value::ReturnException:
					{
						auto error = (ReturnException *)current_exception;

						if(error->target == frame.code)
						{
							value_t result = error->value;
							set_current_exception(0);
							return result;
						}
					}
					break;

					default:
						break;
				}

				return value_raise;
			}

			if(current_exception->get_type() == Value::Exception)
			{
				for(current_handler = 0; current_handler < current_exception_block->handlers.size(); ++current_handler)
				{
					switch(current_exception_block->handlers[current_handler]->type)
					{
						case FilterException:
						{
							auto handler = (FilterExceptionHandler *)current_exception_block->handlers[current_handler];

							ip = ip_start + handler->test_label.address;
							OpContinue; // Execute test block

exception_block_handler: // The test block will jump back here
							auto klass = try_cast<Class>(vars[handler->result]);
							if(!klass || !kind_of(klass, current_exception))
								continue;
						}

						// Fallthrough

						case StandardException:
						{
							if(!kind_of(context->standard_error, current_exception))
								continue;
							else
								break;
						}
					}
					
					auto handler = current_exception_block->handlers[current_handler];

					current_exception_block = current_exception_block->parent;
					set_current_exception(0);

					if(handler->var != no_var)
						vars[handler->var] = current_exception;

					ip = ip_start + handler->rescue_label.address;
					OpContinue; // Execute rescue block
				}
			}
			else if(current_exception_block->loop)
			{
				switch(current_exception->get_type())
				{
					case Value::BreakException:
					{
						set_current_exception(0);
						
						ip = ip_start + current_exception_block->loop->break_label.address;
						OpContinue; // Exit loop
					}
					break;

					case Value::RedoException:
					{
						auto error = (RedoException *)current_exception;

						set_current_exception(0);

						ip = ip_start + error->pos;
						OpContinue; // Restart loop
					}
					break;

					case Value::NextException:
					{
						set_current_exception(0);

						ip = ip_start + current_exception_block->loop->next_label.address;
						OpContinue; // Try next iteration
					}
					break;

					default:
						break;
				}
			}

			
			ExceptionBlock *block = current_exception_block;
			current_exception_block = current_exception_block->parent;

			if(block->ensure_label.address != (size_t)-1)
			{
				ip = ip_start + block->ensure_label.address;
				OpContinue; // Execute ensure block
			}
			else
				goto handle_exception; // Handle parent frame
		}
	}
};
