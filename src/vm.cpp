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

	CharArray Frame::inspect()
	{
		CharArray result = "  in ";

		OnStack<1> os(result);

		result += inspect_obj(obj) + ".";

		value_t module = auto_cast(this->module);

		if(Value::type(module) == Value::IClass)
			module = cast<Module>(module)->instance_of;

		result += inspect_obj(module);

		value_t class_of = real_class_of(obj);

		if(class_of != module)
			result += "(" + inspect_obj(class_of) + ")";

		result += "#" + name->string  + "(";

		for(size_t i = 0; i < argc; ++i)
		{
			result += inspect_obj(argv[i]);
			if(i < argc - 1)
				result += ", ";
		}
			
		result += ")";

		if(code->executor == &evaluate_block)
		{
			Range *range = code->source_location.get((size_t)(ip - code->opcodes));

			if(range)
			{
				CharArray prefix = code->document->name + ":" + CharArray::uint(range->line + 1) + ": ";
				result += "\n" + prefix + range->get_line() + "\n" +  CharArray(" ") * prefix.size() + range->indicator();
			}
			else
				result += "\n" + code->document->name + ":unknown";
		}

		return result;
	}

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
		auto &proc_frame = *(ProcFrame *)&frame;
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

		for(size_t i = 0; i < frame.code->var_words; ++i)
			vars[i] = value_nil;

		OpPrologue

		Op(LoadArg)
			vars[op.var] = frame.argv[op.arg];
		EndOp

		Op(Move)
			vars[op.dst] = vars[op.src];
		EndOp

		Op(Load)
			vars[op.var] = op.imm;
		EndOp

		DeepOp(Call)
			value_t block = op.block ? vars[op.block_var] : value_nil;

			value_t result = call(vars[op.obj], op.method, block, op.argc, &vars[op.argv]);

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

		DeepOp(Class)
			value_t super = op.super == no_var ? Object::class_ref : vars[op.super];

			value_t self = Support::define_class(frame.obj, op.name, super);

			value_t result = call_code(op.block, self, op.name, self, value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Module)
			value_t self = Support::define_module(frame.obj, op.name);

			value_t result = call_code(op.block, self, op.name, self, value_nil, 0, nullptr);

			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(Super)
			value_t block = op.block ? vars[op.block_var] : value_nil;

			value_t module = frame.module;

			Block *method = lookup_super(module, frame.name, &module);

			if(prelude_unlikely(!method))
				goto handle_exception;

			value_t result = call_code(method, frame.obj, frame.name, module, block, op.argc, &vars[op.argv]);

			if(prelude_unlikely(result == value_raise))
				goto handle_call_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		Op(Method)
			Support::define_method(frame.obj, op.name, op.block);
		EndOp

		Op(Lookup)
			vars[op.var] = (value_t)proc_frame.scopes[op.index];
		EndOp

		Op(Self)
			vars[op.var] = frame.obj;
		EndOp

		Op(Block)
			vars[op.var] = frame.block;
		EndOp

		Op(CreateHeap)
			Tuple &heap = Collector::allocate_tuple(op.vars);

			for(size_t i = 0; i < op.vars; ++i)
				heap[i] = value_nil;

			vars[op.var] = &heap;
		EndOp

		Op(GetHeapVar)
			Tuple &heap = *static_cast<Tuple *>(vars[op.heap]);

			vars[op.var] = heap[op.index];
		EndOp

		Op(SetHeapVar)
			Tuple &heap = *static_cast<Tuple *>(vars[op.heap]);

			heap[op.index] = vars[op.var];
		EndOp

		Op(GetIVar)
			vars[op.var] = Support::get_ivar(frame.obj, op.name);
		EndOp

		Op(SetIVar)
			Support::set_ivar(frame.obj, op.name,  vars[op.var]);
		EndOp

		DeepOp(GetConst)
			value_t result = Support::get_const(vars[op.obj], op.name);
		
			if(prelude_unlikely(result == value_raise))
				goto handle_exception;

			if(op.var != no_var)
				vars[op.var] = result;
		EndOp

		DeepOp(SetConst)
			if((prelude_unlikely(Support::set_const(vars[op.obj], op.name,  vars[op.var]) == value_raise)))
				goto handle_exception;
		EndOp

		Op(Array)
			vars[op.var] = Support::create_array(op.argc, &vars[op.argv]);
		EndOp

		Op(String)
			vars[op.var] = Support::create_string((const char *)op.str);
		EndOp

		Op(Interpolate)
			vars[op.var] = Support::interpolate(op.argc, &vars[op.argv]);
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
			set_current_exception(new ReturnException(Value::ReturnException, LocalJumpError::class_ref, String::from_literal("Unhandled return from block"), backtrace().to_string(), op.code, vars[op.var]));
			goto handle_exception;
		EndOp

		DeepOp(UnwindBreak)
			set_current_exception(new BreakException(LocalJumpError::class_ref, String::from_literal("Unhandled break from block"), backtrace().to_string(), op.code, vars[op.var], op.parent_dst));
			goto handle_exception;
		EndOp

		Op(Return)
			value_t result = vars[op.var];
			finalize();
			return result;
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
				finalize();

				if(exception->get_type() == Value::ReturnException)
				{
					auto error = (ReturnException *)exception;

					if(error->target == frame.code)
					{
						value_t result = error->value;
						set_current_exception(0);
						return result;
					}
				}

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

			if(block->ensure_label.block)
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