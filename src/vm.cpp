#include "vm.hpp"
#include "runtime.hpp"
#include "support.hpp"
#include "document.hpp"
#include "generic/range.hpp"
#include "classes/exceptions.hpp"
#include "classes/string.hpp"
#include "classes/module.hpp"

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

		bool handling_exception = false;
		ExceptionBlock *current_exception_block = nullptr;

#ifdef __GNUC__
		value_t storage[frame.code->var_words];
		value_t *vars = storage;

		auto finalize = [&] {};
#else
		value_t *vars = new value_t[frame.code->var_words];

		auto finalize = [&] {
			delete[] vars;
		};
#endif

		frame.vars = vars;

		for(size_t i = 0; i < frame.code->var_words; ++i)
			vars[i] = value_nil;

		OpPrologue

		Op(LoadArg)
			vars[op.var] = frame.argv[op.arg];
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
			finalize();
			return result;
		EndOp

		DeepOp(Call)
#ifdef DEBUG
			for(size_t i = 0; i < frame.code->var_words; ++i)
				mirb_debug_assert(vars[i] != nullptr);
#endif

			value_t block = op.block ? vars[op.block_var] : value_nil;
			value_t obj = vars[op.obj];
			Symbol *name = op.method;
			
			Module *module;

			Method *method = lookup(obj, name, &module);

			if(prelude_unlikely(!method))
				goto handle_call_exception;

			value_t result = call_code(method->block, obj, name, module, block, op.argc, &vars[op.argv]);

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

		Op(BranchIfZero)
			if((size_t)vars[op.var] == 0)
				ip = ip_start + op.pos;
		EndOp

		Op(BranchUnlessZero)
			if((size_t)vars[op.var] != 0)
				ip = ip_start + op.pos;
		EndOp

		Op(Closure)
			vars[op.var] = Support::create_closure(op.block, frame.obj, frame.name, frame.module, op.argc, &vars[op.argv]);
		EndOp
			
		Op(LoadObject)
			vars[op.var] = auto_cast(context->object_class);
		EndOp

		DeepOp(Class)
			value_t super = op.super == no_var ? context->object_class : vars[op.super];
		
			Module *self = define_class(frame.module, op.name, auto_cast(super));

			value_t result = call_code(op.block, self, op.name, self, value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Module)
			Module *self = define_module(frame.module,  op.name);

			value_t result = call_code(op.block, self, op.name, self, value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Super)
			value_t block = op.block ? vars[op.block_var] : value_nil;

			Module *module = frame.module;

			Method *method = lookup_super(module, frame.name, &module);

			if(prelude_unlikely(!method))
				goto handle_exception;

			value_t result = call_code(method->block, frame.obj, frame.name, module, block, op.argc, &vars[op.argv]);

			if(prelude_unlikely(result == value_raise))
				goto handle_call_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp
			
		Op(Method)
			Support::define_method(frame.module, op.name, op.block);
		EndOp

		DeepOp(SingletonMethod)
			if(prelude_unlikely(!Support::define_singleton_method(vars[op.singleton], op.name, op.block)))
				goto handle_exception;
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

		DeepOp(GetConst)
			value_t result = get_const(vars[op.obj], op.name);
		
			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SetConst)
			if((prelude_unlikely(set_const(vars[op.obj], op.name,  vars[op.var]) == value_raise)))
				goto handle_exception;
		EndOp
			
		Op(Array)
			vars[op.var] = Support::create_array(op.argc, &vars[op.argv]);
		EndOp

		Op(Hash)
			vars[op.var] = Support::create_hash(op.argc, &vars[op.argv]);
		EndOp

		Op(String)
			vars[op.var] = CharArray(op.str.data, op.str.length).to_string();
		EndOp

		Op(Interpolate)
			vars[op.var] = Support::interpolate(op.argc, &vars[op.argv], op.result);
		EndOp

		Op(Handler)
			current_exception_block = op.block;
		EndOp

		Op(Unwind)
			if(prelude_unlikely(handling_exception))
			{
				handling_exception = false;
				goto handle_exception;
			}
		EndOp

		DeepOp(UnwindReturn)
			set_current_exception(Collector::allocate<ReturnException>(Value::ReturnException, context->local_jump_error, String::from_literal("Unhandled return from block"), backtrace(), op.code, vars[op.var]));
			goto handle_exception;
		EndOp

		DeepOp(UnwindBreak)
			set_current_exception(Collector::allocate<BreakException>(context->local_jump_error, String::from_literal("Unhandled break from block"), backtrace(), op.code, vars[op.var], op.parent_dst));
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
			vars[op.var] = context->globals.try_get(op.name, [&] { return value_nil; });
		EndOp

		Op(SetGlobal)
			context->globals.set(op.name, vars[op.var]);
		EndOp

		OpEpilogue

handle_call_exception:
		{
			BreakException *exception = (BreakException *)current_exception;

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
			Exception *exception = current_exception;

			if(!current_exception_block)
			{
				switch(exception->get_type())
				{
					case Value::RedoException:
					{
						auto error = (RedoException *)exception;

						set_current_exception(0);

						ip =  ip_start + error->pos;
						OpContinue; // Restart block
					}
					break;

					case Value::NextException:
					{
						auto error = (NextException *)exception;

						value_t result = error->value;
						set_current_exception(0);
						finalize();
						return result;
					}
					break;

					case Value::ReturnException:
					{
						auto error = (ReturnException *)exception;

						if(error->target == frame.code)
						{
							value_t result = error->value;
							set_current_exception(0);
							finalize();
							return result;
						}
					}
					break;

					default:
						break;
				}

				finalize();

				return value_raise;
			}

			ExceptionBlock *block = current_exception_block;

			if(exception->get_type() == Value::Exception)
			{
				for(auto i = block->handlers.begin(); i != block->handlers.end(); ++i)
				{
					switch(i()->type)
					{
						case RuntimeException:
						{
							current_exception_block = block->parent;
							set_current_exception(0);
							ip =  ip_start + ((RuntimeExceptionHandler *)i())->rescue_label.address;
							OpContinue; // Execute rescue block
						}

						default:
							break;
					}
				}
			}

			current_exception_block = block->parent;

			if(block->ensure_label.address != (size_t)-1)
			{
				handling_exception = true;
				ip =  ip_start + block->ensure_label.address;
				OpContinue; // Execute ensure block
			}
			else
				goto handle_exception; // Handle parent frame
		}
	}
};
