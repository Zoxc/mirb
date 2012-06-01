#include "exceptions.hpp"
#include "../runtime.hpp"
#include "../parser/parser.hpp"
#include "../platform/platform.hpp"
#include "../document.hpp"
#include "class.hpp"
#include "io.hpp"
#include "string.hpp"

namespace Mirb
{
	SystemExit::SystemExit(int result) : Exception(Type::SystemExit, context->system_exit_class, 0, Mirb::backtrace()), result(result)
	{
	}
	
	SyntaxError::SyntaxError(const Parser &parser) : Exception(Type::SyntaxError, context->syntax_error, String::get("Unable to parse file '" + parser.document.name + "'"), Mirb::backtrace())
	{
		messages = parser.messages;
	}
	
	value_t SyntaxError::print(Exception *self, IO *io)
	{
		OnStack<2> os(self, io);

		print_main(self, io);

		auto syntax_error = try_cast<SyntaxError>(self);
		
		if(syntax_error)
		{
			for(auto message: syntax_error->messages)
			{
				io->stream->print("\n");

				message->print(*io->stream);
			}
		}

		print_backtrace(self, io);

		return value_nil;
	}

	SystemStackError::SystemStackError() : Exception(context->system_stack_error, String::get("Stack oveflow"), Mirb::backtrace())
	{
	}

	void initialize_exceptions()
	{
		context->standard_error = define_class("StandardError", context->exception_class);
		
		context->system_exit_class = define_class("SystemExit", context->exception_class);
		context->system_stack_error = define_class("SystemStackError", context->exception_class);
		
		context->io_error = define_class("IOError", context->standard_error);
		context->name_error = define_class("NameError", context->standard_error);
		context->type_error = define_class("TypeError", context->standard_error);
		context->syntax_error = define_class("SyntaxError", context->standard_error);
		context->argument_error = define_class("ArgumentError", context->standard_error);
		context->zero_division_error = define_class("ZeroDivisionError", context->standard_error);
		context->runtime_error = define_class("RuntimeError", context->standard_error);
		context->local_jump_error = define_class("LocalJumpError", context->standard_error);
		context->load_error = define_class("LoadError", context->standard_error);
		context->system_call_error = define_class("SystemCallError", context->standard_error);

		context->signal_exception = define_class("SignalException", context->exception_class);
		context->interrupt_class = define_class("Interrupt", context->signal_exception);

		method<Arg::Self<Arg::Class<Exception>>, Arg::Class<IO>, &SyntaxError::print>(context->syntax_error, "print");
	}
};

