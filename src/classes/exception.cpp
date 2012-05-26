#include "exception.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "../runtime.hpp"
#include "../collector.hpp"
#include "class.hpp"
#include "../block.hpp"
#include "../document.hpp"
#include "../compiler.hpp"
#include "../generic/source-loc.hpp"
#include "../platform/platform.hpp"
#include "../vm.hpp"

namespace Mirb
{
	StackFrame::StackFrame(Frame *frame) :
		Value::Header(Value::InternalStackFrame),
		code(frame->code),
		obj(frame->obj),
		name(frame->name),
		scope(frame->scope),
		block(frame->block),
		ip(frame->ip),
		vars(frame->vars != nullptr)
	{
		args = Tuple<>::allocate(frame->argc);

		for(size_t i = 0; i < frame->argc; ++i)
			(*args)[i] = frame->argv[i];
	}
	
	void StackFrame::print(StackFrame *self)
	{
		OnStack<1> os(self);

		Platform::color<Platform::Gray>("  in ");

		Module *module = self->scope->first();

		if(Value::type(module) == Value::IClass)
			module = module->original_module;

		OnStack<1> os3(module);

		Platform::color<Platform::Gray>(inspect_obj(module));

		value_t class_of = real_class_of(self->obj);

		if(class_of != module)
		{
			Platform::color<Platform::Gray>("(");
			Platform::color<Platform::Gray>(inspect_object(class_of));
			Platform::color<Platform::Gray>(")");
		}

		Platform::color<Platform::Gray>("#");

		std::cerr << self->name->get_string()  << "(";

		for(size_t i = 0; i < self->args->entries; ++i)
		{
			std::cerr << inspect_object((*self->args)[i]);
			if(i < self->args->entries - 1)
				std::cerr << ", ";
		}
			
		std::cerr <<  ")";

		if(self->code->executor == &evaluate_block || self->code->executor == &Compiler::deferred_block)
		{
			SourceLoc *range;

			if(self->vars)
				range = self->code->source_location.get((size_t)(self->ip - self->code->opcodes));
			else
				range = self->code->range;

			if(range)
			{
				CharArray prefix = self->code->document->name + "[" + CharArray::uint(range->line + 1) + "]: ";
				
				Platform::color<Platform::Bold>("\n" + prefix);

				std::cerr << range->get_line() << "\n";
				
				Platform::color<Platform::Green>(CharArray(" ") * prefix.size() + range->indicator());
			}
			else
				Platform::color<Platform::Bold>("\n" + self->code->document->name + "[?]: Unknown");
		}
	}
	
	CharArray StackFrame::inspect_implementation(StackFrame *self)
	{
		OnStack<1> os(self);

		CharArray result = "  in ";

		OnStackString<1> os2(result);
		
		Module *module = self->scope->first();

		if(Value::type(module) == Value::IClass)
			module = module->original_module;

		OnStack<1> os3(module);

		result += inspect_obj(module);

		value_t class_of = real_class_of(self->obj);

		if(class_of != module)
		{
			result += "(";
			result += inspect_obj(class_of) + ")";
		}

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
			SourceLoc *range;

			if(self->vars)
				range = self->code->source_location.get((size_t)(self->ip - self->code->opcodes));
			else
				range = self->code->range;

			if(range)
			{
				CharArray prefix = self->code->document->name + "[" + CharArray::uint(range->line + 1) + "]: ";
				result += "\n" + prefix + range->get_line() + "\n" +  CharArray(" ") * prefix.size() + range->indicator();
			}
			else
				result += "\n" + self->code->document->name + "[?]: Unknown";
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
	
	void StackFrame::print_backtrace(Tuple<StackFrame> *backtrace)
	{
		if(!backtrace)
			return;
		
		OnStack<1> os1(backtrace);
		
		for(size_t i = 0; i < backtrace->entries; ++i)
		{
			if(i != 0)
				std::cerr << "\n";

			print((*backtrace)[i]);
		}
	}
	
	value_t Exception::allocate(Class *instance_of)
	{
		return Collector::allocate<Exception>(Value::Exception, instance_of, nullptr, nullptr);
	}
	
	value_t Exception::to_s(Exception *self)
	{
		return self->message;
	}

	value_t Exception::rb_backtrace(Exception *self)
	{
		if(self->backtrace)
			return StackFrame::get_backtrace(self->backtrace).to_string();
		else
			return value_nil;
	}

	value_t Exception::rb_initialize(Exception *self, String *message)
	{
		self->message = message;

		return self;
	}

	void Exception::initialize()
	{
		context->exception_class = define_class("Exception", context->object_class);

		singleton_method<Arg::Self<Arg::Class<Class>>>(context->exception_class, "allocate", &allocate);

		method<Arg::Self<Arg::Class<Exception>>, Arg::Class<String>>(context->exception_class, "initialize", &rb_initialize);
		method<Arg::Self<Arg::Class<Exception>>>(context->exception_class, "backtrace", &rb_backtrace);
		method<Arg::Self<Arg::Class<Exception>>>(context->exception_class, "message", &to_s);
		method<Arg::Self<Arg::Class<Exception>>>(context->exception_class, "to_s", &to_s);
	}
};

