#include "exception.hpp"
#include "symbol.hpp"
#include "string.hpp"
#include "array.hpp"
#include "io.hpp"
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
	
	void StackFrame::print(StackFrame *self, Stream &out)
	{
		OnStack<1> os(self);

		out.color(Gray, "  in ");

		Module *module = self->scope->first();

		if(Value::type(module) == Value::IClass)
			module = module->original_module;

		OnStack<1> os3(module);

		out.color(Gray, Mirb::inspect(module));

		value_t class_of_obj = class_of(self->obj);

		if(class_of_obj != module)
		{
			out.color(Gray, "(");
			out.color(Gray, Mirb::inspect(class_of_obj));
			out.color(Gray, ")");
		}

		out.color(Gray, "#");

		out.print(self->name->string + "(");

		for(size_t i = 0; i < self->args->entries; ++i)
		{
			out.print(Mirb::inspect((*self->args)[i]));
			if(i < self->args->entries - 1)
				out.print(", ");
		}
			
		out.print(")");

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
				
				out.color(Bold, "\n" + prefix);

				out.print(range->get_line() + "\n");
				
				out.color(Green, CharArray(" ") * prefix.size() + range->indicator());
			}
			else
				out.color(Bold, "\n" + self->code->document->name + "[?]: Unknown");
		}
	}
	
	CharArray StackFrame::inspect_plain_implementation(StackFrame *self)
	{
		CharArray result;

		if(self->code->executor == &evaluate_block)
		{
			SourceLoc *range;

			if(self->vars)
				range = self->code->source_location.get((size_t)(self->ip - self->code->opcodes));
			else
				range = self->code->range;

			if(range)
			{
				result += self->code->document->name + ":" + CharArray::uint(range->line + 1) + ":";
			}
			else
				result += self->code->document->name + ":unknown:";
		}
		else
				result += "mirb:native:";

		result += "in `" + self->name->string + "'";

		return result;
	}
	
	CharArray StackFrame::inspect()
	{
		CharArray result;

		CharArrayStream stream(result);

		print(this, stream);

		return result;
	}
	
	CharArray StackFrame::inspect_plain()
	{
		return inspect_plain_implementation(this);
	}
	
	Array *StackFrame::get_plain_backtrace(Tuple<StackFrame> *backtrace)
	{
		auto result = Array::allocate();

		if(!backtrace)
			return result;

		OnStack<2> os(backtrace, result);
		
		for(size_t i = 0; i < backtrace->entries; ++i)
		{
			result->vector.push((*backtrace)[i]->inspect_plain().to_string());
		}

		return result;
	}
	
	void StackFrame::print_backtrace(Tuple<StackFrame> *backtrace, Stream &out)
	{
		if(!backtrace)
			return;
		
		OnStack<1> os1(backtrace);
		
		for(size_t i = 0; i < backtrace->entries; ++i)
		{
			if(i != 0)
				out.print("\n");

			print((*backtrace)[i], out);
		}
	}
	
	value_t Exception::allocate(Class *instance_of)
	{
		return Collector::allocate<Exception>(Value::Exception, instance_of, nullptr, nullptr);
	}
	
	value_t Exception::to_s(Exception *self)
	{
		if(self->message)
			return self->message;
		else
			return value_nil;
	}
	
	void Exception::print_main(Exception *self, IO *io)
	{
		io->assert_stream();

		OnStack<2> os(self, io);
		
		io->stream->color(Red, inspect(class_of(self)));

		if(self->message)
		{
			io->stream->print(": ");
			io->stream->print(self->message->string);
		}
	}
	
	void Exception::print_backtrace(Exception *self, IO *io)
	{
		if(self->backtrace)
		{
			io->assert_stream();
			
			io->stream->print("\n");

			StackFrame::print_backtrace(self->backtrace, *io->stream);
		}
	}

	value_t Exception::print(Exception *self, IO *io)
	{
		io->assert_stream();

		OnStack<2> os(self, io);
		
		print_main(self, io);
		print_backtrace(self, io);
		
		return value_nil;
	}

	value_t Exception::rb_backtrace(Exception *self)
	{
		if(self->backtrace)
			return StackFrame::get_plain_backtrace(self->backtrace);
		else
			return value_nil;
	}

	value_t Exception::rb_initialize(Exception *self, String *message)
	{
		self->message = message;

		return self;
	}
	
	value_t Exception::rb_inspect(Exception *self)
	{
		OnStack<1> os(self);

		CharArray klass = inspect(class_of(self));

		CharArray result = "#<" + klass + (self->message ? (": " + self->message->string) : "") + ">";

		return result.to_string();
	}

	void Exception::initialize()
	{
		context->exception_class = define_class("Exception", context->object_class);

		singleton_method<Arg::Self<Arg::Class<Class>>, &allocate>(context->exception_class, "allocate");

		method<Arg::Self<Arg::Class<Exception>>, Arg::Optional<Arg::Class<String>>, &rb_initialize>(context->exception_class, "initialize");
		method<Arg::Self<Arg::Class<Exception>>, &rb_backtrace>(context->exception_class, "backtrace");
		method<Arg::Self<Arg::Class<Exception>>, &to_s>(context->exception_class, "message");
		method<Arg::Self<Arg::Class<Exception>>, &to_s>(context->exception_class, "to_s");
		method<Arg::Self<Arg::Class<Exception>>, &rb_inspect>(context->exception_class, "inspect");
		method<Arg::Self<Arg::Class<Exception>>, Arg::Class<IO>, &print>(context->exception_class, "print");
	}
};

