#include "exception.hpp"
#include "symbol.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"
#include "class.hpp"
#include "../block.hpp"
#include "../document.hpp"
#include "../generic/range.hpp"
#include "../vm.hpp"

namespace Mirb
{
	StackFrame::StackFrame(Frame *frame) :
		Value::Header(Value::InternalStackFrame),
		code(frame->code),
		obj(frame->obj),
		name(frame->name),
		module(frame->module),
		block(frame->block),
		ip(frame->ip)
	{
		args = Tuple<>::allocate(frame->argc);

		for(size_t i = 0; i < frame->argc; ++i)
			(*args)[i] = frame->argv[i];
	}
	
	CharArray StackFrame::inspect_implementation(StackFrame *self)
	{
		OnStack<1> os(self);

		CharArray result = "  in ";

		OnStackString<1> os2(result);

		value_t module = auto_cast(self->module);

		if(Value::type(module) == Value::IClass)
			module = cast<Module>(module)->instance_of;
		
		OnStack<1> os3(module);

		result += inspect_obj(module);

		value_t class_of = real_class_of(self->obj);

		if(class_of != module)
			result += "(" + inspect_obj(class_of) + ")";

		result += "#" + self->name->string  + "(";

		for(size_t i = 0; i < self->args->entries; ++i)
		{
			result += inspect_obj((*self->args)[i]);
			if(i < self->args->entries - 1)
				result += ", ";
		}
			
		result += ")";

		if(self->code->executor == &evaluate_block)
		{
			Range *range = self->code->source_location.get((size_t)(self->ip - self->code->opcodes));

			if(range)
			{
				CharArray prefix = self->code->document->name + ":" + CharArray::uint(range->line + 1) + ": ";
				result += "\n" + prefix + range->get_line() + "\n" +  CharArray(" ") * prefix.size() + range->indicator();
			}
			else
				result += "\n" + self->code->document->name + ":unknown";
		}

		return result;
	}
	
	CharArray StackFrame::inspect()
	{
		return inspect_implementation(this);
	}
	
	CharArray StackFrame::get_backtrace(Tuple<StackFrame> *backtrace)
	{
		CharArray result;

		if(!backtrace)
			return result;
		
		OnStack<1> os1(backtrace);
		OnStackString<1> os2(result);
		
		for(size_t i = 0; i < backtrace->entries; ++i)
		{
			if(i != 0)
				result += "\n";

			result += (*backtrace)[i]->inspect();
		}

		return result;
	}
	
	value_t Exception::allocate(value_t obj)
	{
		return auto_cast(Collector::allocate<Exception>(Value::Exception, auto_cast(obj), value_nil, nullptr));
	}

	value_t Exception::to_s(value_t obj)
	{
		auto self = cast<Exception>(obj);

		return self->message;
	}

	value_t Exception::method_initialize(value_t obj, value_t message)
	{
		auto self = cast<Exception>(obj);

		self->message = message;

		return obj;
	}

	void Exception::initialize()
	{
		context->exception_class = define_class(context->object_class, "Exception", context->object_class);

		singleton_method<Arg::Self>(context->exception_class, "allocate", &allocate);

		method<Arg::Self, Arg::Value>(context->exception_class, "initialize", &method_initialize);
		method<Arg::Self>(context->exception_class, "message", &to_s);
		method<Arg::Self>(context->exception_class, "to_s", &to_s);
	}
};

